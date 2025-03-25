#include "transport_router.hpp"
#include "transport_catalogue.hpp"

namespace transport_router
{
	void TransportRouter::Init(TransportRouterInitList&& init) {
		bus_velocity_ = init.bus_velocity;
		bus_wait_time_ = init.bus_wait_time;
		catalogue_ = init.catalogue;

		graph_ = ::graph::DirectedWeightedGraph<double>{ catalogue_->GetStopsCount() };
		SetGraphWithRoutes();
		router_uptr_.reset(new ::graph::Router<double>{ graph_ });
	}

	std::optional<const RouteInfo* const> TransportRouter::GetRouteInfo(std::string_view from, std::string_view to) {
		auto stop_from_ptr{ catalogue_->GetStopPtr(from) };
		auto stop_to_ptr{ catalogue_->GetStopPtr(to) };
		std::optional<size_t> vertex_from{ wrapper_uptr_->WrapVertex(stop_from_ptr) };
		std::optional<size_t> vertex_to{ wrapper_uptr_->WrapVertex(stop_to_ptr) };

		if (!vertex_from || !vertex_to) {
			return std::nullopt;
		}

		if (routes_info_.contains({ vertex_from.value(), vertex_to.value() })) {
			return &(routes_info_[{ vertex_from.value(), vertex_to.value()}]);
		}

		return CreateAndSaveNewRouteInfo(vertex_from.value(), vertex_to.value());
	}

	std::optional<const RouteInfo* const> TransportRouter::CreateAndSaveNewRouteInfo(size_t from, size_t to) {
		auto raw_info{ router_uptr_->BuildRoute(from, to) };
		if (!raw_info) {
			return std::nullopt;
		}

		std::vector<Item> items;
		for (const auto edge_id : raw_info.value().edges) {
			auto item_info = wrapper_uptr_->UnwrapEdge(edge_id);
			WaitItem wait_item{ .stop_name = item_info.wait_stop_name, .time = bus_wait_time_ };
			items.push_back({ std::move(wait_item) });
			BusItem bus_item{ .bus = item_info.bus_name, .span_count = item_info.span_count, .time = item_info.time };
			items.push_back({ std::move(bus_item) });
		}

		routes_info_.insert(
			{ { from, to }
			, RouteInfo{.total_time = raw_info.value().weight, .items = std::move(items) }
			});

		return std::optional<const RouteInfo* const>{ &routes_info_.at({ from, to }) };
	}

	void TransportRouter::SetGraphWithRoutes() {
		auto builder = Wrapper::Builder<Catalogue, Wrapper>{
			{
				.catalogue = *catalogue_,
				.graph = graph_,
				.bus_wait_time = bus_wait_time_,
				.bus_velocity = bus_velocity_
			}
		};
		wrapper_uptr_ = builder.Make();
	}
}//transport_router