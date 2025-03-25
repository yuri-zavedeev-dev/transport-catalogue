#pragma once

#include "graph.hpp"
#include "router.hpp"
#include "domain.hpp"

#include <vector>
#include <unordered_map>
#include <string_view>
#include <memory>
#include <optional>
#include <variant>

namespace transport_catalogue
{
	class TransportCatalogue;
}//transport_catalogue

namespace transport_router
{
	struct WaitItem {
		std::string_view stop_name;
		unsigned int time;
	};

	struct BusItem {
		std::string_view bus;
		unsigned int span_count;
		double time;
	};

	struct Item {
		std::variant<WaitItem, BusItem> content;
	};

	struct RouteInfo final {
		double total_time;
		std::vector<Item> items;
	};

	struct SizeTPairHasher {
		auto operator() (const std::pair<size_t, size_t>& p) const -> size_t {
			return std::hash<size_t>{}(p.first) * 47
				+ std::hash<size_t>{}(p.second) * 47 * 47;
		}
	};

	class TransportRouter final {
	private:
		using Catalogue = transport_catalogue::TransportCatalogue;
		using Wrapper = ::transport_catalogue::size_t_wrapper::Wrapper;

	public:
		TransportRouter() = default;

		struct TransportRouterInitList {
			const unsigned int bus_wait_time;
			const unsigned int bus_velocity;
			const Catalogue* catalogue;
		};

		void Init(TransportRouterInitList&& init);
		std::optional<const RouteInfo* const> GetRouteInfo(std::string_view from, std::string_view to);

	private:
		std::optional<const RouteInfo* const> CreateAndSaveNewRouteInfo(size_t from, size_t to);
		void SetGraphWithRoutes();

		unsigned int bus_wait_time_;
		unsigned int bus_velocity_;
		const Catalogue* catalogue_;
		::graph::DirectedWeightedGraph<double> graph_;
		std::unique_ptr<::graph::Router<double>> router_uptr_{ nullptr };
		std::unique_ptr<Wrapper> wrapper_uptr_;

		std::unordered_map<std::pair<size_t, size_t>, RouteInfo, SizeTPairHasher> routes_info_;
	};
} //transport_router