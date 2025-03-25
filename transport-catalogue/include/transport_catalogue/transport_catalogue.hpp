#pragma once

#include <algorithm>
#include <forward_list>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "domain.hpp"
#include "geo.hpp"
#include "json.hpp"
#include "transport_router.hpp"

namespace transport_catalogue
{	
	class TransportCatalogue {
		friend class size_t_wrapper::Wrapper::Builder<TransportCatalogue, size_t_wrapper::Wrapper>;

	public:
		using Vertex = const details::Stop* const;
		using VertexHasher = details::StopPtrPairHasher;
		using Cell = unsigned long;
		using AdjacencyMatrix = std::unordered_map<std::pair<Vertex, Vertex>, Cell, VertexHasher>;

	private:
		using Router = transport_router::TransportRouter;

	public:
		TransportCatalogue() = default;
		~TransportCatalogue();

		void AddStop(std::string&& name, geo::Coordinates&& location);
		void AddRoute(std::string&& name, std::vector<std::string_view>&& stops_names);

		const details::Stop& GetStop(std::string_view name) const;
		const details::Stop* GetStopPtr(std::string_view name) const;
		size_t GetStopsCount() const;
		const details::Route& GetRoute(std::string_view name) const;

		const details::RouteInfo& GetRouteInfo(std::string_view name);
		const details::StopInfo& GetStopInfo(std::string_view name);

		void SetDistanceBetweenStops(
			const details::Stop* const stop_from
			, const details::Stop* const stop_to
			, unsigned long distance
		);
		unsigned long GetDistanceBetweenStops(
			const details::Stop* const stop_from
			, const details::Stop* const stop_to
		) const;

		void InitRouter(Router::TransportRouterInitList&& init);
		std::optional<const transport_router::RouteInfo* const> BuildRoute(
			std::string_view from
			, std::string_view to
		);

	private:
		details::RouteInfo& SaveNewRouteInfo(std::string_view name);
		[[nodiscard]] details::RouteInfo CreateRouteInfo(const details::Route* route_ptr);

		details::StopInfo& SaveNewStopInfo(std::string_view name);
		[[nodiscard]] details::StopInfo CreateStopInfo(const details::Stop* stop_ptr);

		std::unordered_map<std::string_view, details::Stop> unique_stops_;
		std::unordered_map<std::string_view, details::Route> unique_routes_;

		std::unordered_map<std::string_view, details::RouteInfo> routes_info_;
		std::unordered_map<std::string_view, details::StopInfo> stops_info_;

		std::unordered_map<
			std::string_view
			, std::unordered_set<details::Route*, details::RoutePtrHasher>
		> stops_to_routes_;

		AdjacencyMatrix distance_graph_;
		Router router_;

		std::unordered_set<std::string> unique_names_;
	};
}//transport_catalogue