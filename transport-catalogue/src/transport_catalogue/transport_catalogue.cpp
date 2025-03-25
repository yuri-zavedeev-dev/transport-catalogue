#include "transport_catalogue.hpp"

namespace transport_catalogue
{

TransportCatalogue::~TransportCatalogue() {
}

void TransportCatalogue::AddStop(std::string&& name, geo::Coordinates&& location) {
	std::unordered_set<std::string>::iterator it = unique_names_.insert(std::move(name)).first;
	unique_stops_[std::string_view{ *it }] = details::Stop{ std::string_view{ *it }, std::move(location) };
}

void TransportCatalogue::AddRoute(std::string&& route_name, std::vector<std::string_view>&& stops_names) {
	std::forward_list<details::Stop*> stops;
	for (std::string_view stop_name : stops_names) {
		auto it = unique_stops_.find(stop_name);
		if (it == unique_stops_.end()) {
			throw std::logic_error{ "DataBase::AddRoute: No such stop!" };
		}
		stops.push_front(&(it->second));
	}
	std::unordered_set<std::string>::iterator it = unique_names_.insert(std::move(route_name)).first;
	unique_routes_[std::string_view{ *it }] = std::move(details::Route{ std::string_view{ *it }, std::move(stops) });
	for (details::Stop* stop_ptr : unique_routes_[std::string_view{ *it }].stops) {
		stops_to_routes_[stop_ptr->name].insert(&unique_routes_[std::string_view{ *it }]);
	}
}

	const details::Stop& TransportCatalogue::GetStop(std::string_view name) const {
		auto it{ unique_stops_.find(std::move(name)) };
		if (it == unique_stops_.end()) {
			throw std::logic_error{ "DataBase::GetStop: No such stop!" };
		}
		return it->second;
	}

	const details::Stop* TransportCatalogue::GetStopPtr(std::string_view name) const {
		auto it{ unique_stops_.find(std::move(name)) };
		if (it == unique_stops_.end()) {
			throw std::logic_error{ "DataBase::GetStopPtr: No such stop!" };
		}
		return &(it->second);
	}

	size_t TransportCatalogue::GetStopsCount() const {
		return unique_stops_.size();
	}

	const details::Route& TransportCatalogue::GetRoute(std::string_view name) const {
		auto it{ unique_routes_.find(std::move(name)) };
		if (it == unique_routes_.end()) {
			throw std::logic_error{ "DataBase::GetRoute: No such route!" };
		}
		return it->second;
	}


	const details::RouteInfo& TransportCatalogue::GetRouteInfo(std::string_view name) {
		auto route_info_it{ routes_info_.find(name) };
		if (route_info_it == routes_info_.end()) {
			return SaveNewRouteInfo(name);
		}
		return route_info_it->second;
	}

	details::RouteInfo& TransportCatalogue::SaveNewRouteInfo(std::string_view name)
	{
		auto route_it{ unique_routes_.find(name) };
		if (route_it == unique_routes_.end()) {
			throw std::logic_error{ "DataBase::SaveNewRouteInfo: No such route!" };
		}

		auto route_info = CreateRouteInfo(&(route_it->second));
		routes_info_[route_it->first] = std::move(route_info);

		return routes_info_[route_it->first];
	}

	[[nodiscard]] details::RouteInfo TransportCatalogue::CreateRouteInfo(const details::Route* route_ptr) {
		const unsigned short stops_count{ static_cast<unsigned short>(std::distance(route_ptr->stops.begin()
									, route_ptr->stops.end()))
		};

		size_t distance_total{ 0 };
		details::Stop* stop_prev = nullptr;

		std::unordered_set<void*> unique_stops;

		for (details::Stop* stop_ptr : route_ptr->stops) {
			unique_stops.insert(reinterpret_cast<void*>(stop_ptr));

			if (!stop_prev) {
				stop_prev = stop_ptr;
				continue;
			}
 			distance_total += GetDistanceBetweenStops(stop_ptr, stop_prev);
			stop_prev = stop_ptr;
		}
		const unsigned short unique_stops_count{ static_cast<unsigned short>(unique_stops.size()) };

		double distance_total_geographical{ 0. };
		auto it{ route_ptr->stops.begin() };
		auto prev_it{ it };
		it = std::next(it);
		for (; it != route_ptr->stops.end();) {
			distance_total_geographical += geo::ComputeDistance((*prev_it)->location, (*it)->location);
			it = std::next(it);
			prev_it = std::next(prev_it);
		}

		return details::RouteInfo{ route_ptr->name
							, stops_count
							, unique_stops_count
							, distance_total
							, static_cast<double>(distance_total) / distance_total_geographical
		};
	}

	const details::StopInfo& TransportCatalogue::GetStopInfo(std::string_view name) {
		auto stop_info_it{ stops_info_.find(name) };
		if (stop_info_it == stops_info_.end()) {
			return SaveNewStopInfo(name);
		}
		return stop_info_it->second;
	}

	details::StopInfo& TransportCatalogue::SaveNewStopInfo(std::string_view name)
	{
		auto stop_it{ unique_stops_.find(name) };
		if (stop_it == unique_stops_.end()) {
			throw std::logic_error{ "DataBase::SaveNewStopInfo: No such stop!" };
		}

		auto stop_info = CreateStopInfo(&(stop_it->second));
		stops_info_[stop_it->first] = std::move(stop_info);

		return stops_info_[stop_it->first];
	}

	[[nodiscard]] details::StopInfo TransportCatalogue::CreateStopInfo(const details::Stop* stop_ptr) {
		std::unordered_set<details::Route*, details::RoutePtrHasher>* routes_ptrs{ &stops_to_routes_[stop_ptr->name] };
		return details::StopInfo{ stop_ptr->name
			, routes_ptrs
		};
	}

	void TransportCatalogue::SetDistanceBetweenStops(
		const details::Stop* const stop_from
		, const details::Stop* const stop_to
		, unsigned long distance
	) {
		distance_graph_[{ stop_from, stop_to }] = distance;
		if (distance_graph_.find({ stop_to, stop_from }) == distance_graph_.end()) {
			distance_graph_[{ stop_to, stop_from }] = distance;
		}
	}

	unsigned long TransportCatalogue::GetDistanceBetweenStops(
		const details::Stop* const stop_from
		, const details::Stop* const stop_to
	) const {
		if (distance_graph_.find({ stop_to, stop_from }) == distance_graph_.end()) {
			throw std::logic_error{"TtransportCatalogue::GetDistanceBetweenStops: No data presented!"};
		}
		return distance_graph_.at({ stop_from, stop_to });
	}

	void TransportCatalogue::InitRouter(Router::TransportRouterInitList&& init) {
		router_.Init(std::move(init));
	}

	std::optional<const transport_router::RouteInfo* const> TransportCatalogue::BuildRoute(
		std::string_view from
		, std::string_view to
	) {
		return router_.GetRouteInfo(from, to);
	}
}//transport_catalogue