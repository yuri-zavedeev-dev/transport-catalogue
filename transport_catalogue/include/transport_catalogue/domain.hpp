#pragma once
#include <forward_list>
#include <string_view>
#include <unordered_set>
#include <map>
#include <unordered_map>
#include <memory>
#include <optional>

#include "geo.hpp"
#include "graph.hpp"

namespace transport_catalogue
{
	namespace details
	{
		struct Stop {
			Stop() = default;
			Stop(const std::string_view&& name, const geo::Coordinates location);
			std::string_view name;
			geo::Coordinates location;
		};

		struct StopPtrPairHasher {
			auto operator() (std::pair<const Stop* const, const Stop* const> pair_to_hash) const -> size_t {
				return std::hash<std::string_view>{}(pair_to_hash.first->name) * 47
					+ std::hash<std::string_view>{}(pair_to_hash.second->name) * 47 * 47;
			}
		};

		struct Route {
			Route() = default;
			Route(std::string_view&& name, std::forward_list<Stop*>&& stops);
			Route(const std::string_view& name, std::forward_list<Stop*>&& stops);
			~Route() = default;

			std::string_view name;
			std::forward_list<Stop*> stops;
		};

		struct RoutePtrHasher {
			auto operator() (Route* route) const -> size_t {
				return std::hash<std::string_view>{}(route->name);
			}
		};

		struct StopInfo {
			std::string_view name;
			std::unordered_set<Route*, RoutePtrHasher>* routes;
		};

		struct RouteInfo {
			std::string_view name;
			unsigned short stops_count;
			unsigned short unique_stops_count;
			size_t distance_total;
			double curvature;
		};
	}//details

	namespace size_t_wrapper
	{
		class Wrapper {
		private:
			using VertexT = details::Stop;
			using CPtrVertexT = const VertexT* const;

		public:
			struct EdgeInfo {
				std::string_view bus_name;
				std::string_view wait_stop_name;
				unsigned int span_count;
				double time;
			};

			struct StopInfo {
				std::string_view name;
			};

			template<typename Catalogue, typename Wrapper>
			class Builder {
			public:
				struct BuilderInit {
					const Catalogue& catalogue;
					::graph::DirectedWeightedGraph<double>& graph;
					unsigned int bus_wait_time;
					unsigned int bus_velocity;
				};

				Builder(BuilderInit&& init)
					: catalogue_{ init.catalogue }, graph_{ init.graph }
					, bus_wait_time_{ init.bus_wait_time }, bus_velocity_{ init.bus_velocity }
				{}

				std::unique_ptr<Wrapper> Make() {
					for (const auto& [name, route] : catalogue_.unique_routes_) {
						CreateSubRoutes(name, route);
					}

					return std::make_unique<Wrapper>(wrapper_);
				}

			private:
				void CreateSubRoutes(const std::string_view name, const details::Route& route)
				{
					using StopIt = std::forward_list<details::Stop*>::const_iterator;

					std::forward_list<details::Stop*> route_reversed;
					for (details::Stop* stop_ptr : route.stops) {
						route_reversed.push_front(stop_ptr);
					}

					StopIt base_stop_it{ route_reversed.begin() };
					StopIt prev_stop_it{ base_stop_it };
					unsigned int span_count{ 0 };
					unsigned long prev_distance{ 0 };
					for (StopIt stop_it{ base_stop_it }; base_stop_it != route_reversed.end();) {
						stop_it++;
						span_count++;
						if (stop_it == route_reversed.end() || *stop_it == *base_stop_it) {
							base_stop_it++;
							stop_it = base_stop_it;
							prev_stop_it = base_stop_it;
							span_count = 0;
							prev_distance = 0;
							continue;
						}
						
						size_t base_stop_id = GiveStopId(*base_stop_it);
						size_t stop_id = GiveStopId(*stop_it);

						unsigned long new_step{ catalogue_.GetDistanceBetweenStops(*prev_stop_it, *stop_it) };
						double weight{ (new_step + prev_distance)	// m
							/ 1000.									// m -> km
							/ static_cast<double>(bus_velocity_)	// km -> h (km \ km_per_h)
							* 60.									// h -> min
						};
						graph_.AddEdge({
							.from = base_stop_id,
							.to = stop_id,
							.weight = weight + static_cast<double>(bus_wait_time_)
						});

						wrapper_.AddEdge(edge_number_, std::move(EdgeInfo{
								.bus_name = name,
								.wait_stop_name = (*base_stop_it)->name,
								.span_count = span_count,
								.time = weight
							})
						);
						edge_number_++;

						prev_distance += new_step;
						prev_stop_it++;
					}
				}

				size_t GiveStopId(const details::Stop* const stop_ptr) {
					if (wrapper_.vertex_to_id_map_.contains(stop_ptr)) {
						return wrapper_.vertex_to_id_map_.at(stop_ptr);
					}

					wrapper_.vertex_to_id_map_.insert({ stop_ptr, vertex_number_ });
					wrapper_.id_to_vertex_map_.insert({ vertex_number_, StopInfo{ .name = stop_ptr->name } });
					vertex_number_++;
					return vertex_number_ - 1;
				}

				const Catalogue& catalogue_;
				::graph::DirectedWeightedGraph<double>& graph_;
				const unsigned int bus_wait_time_;
				const unsigned int bus_velocity_;
				size_t vertex_number_{ 0 };
				size_t edge_number_{ 0 };
				Wrapper wrapper_{};
			};

			std::optional<size_t> WrapVertex(const VertexT* const ptr) const {
				if (auto it = vertex_to_id_map_.find(ptr); it != vertex_to_id_map_.end()){
					return it->second;
				}

				return std::nullopt;
			}

			const StopInfo& UnwrapVertex(const size_t id) const {
				return id_to_vertex_map_.at(id);
			}

			const EdgeInfo& UnwrapEdge(const size_t edge_number) const {
				return id_to_edge_map_.at(edge_number);
			}

		private:
			Wrapper() = default;

			void AddEdge(const size_t id, const EdgeInfo&& info) {
				id_to_edge_map_.insert({id, std::move(info)});
			}

			std::map<CPtrVertexT, const size_t> vertex_to_id_map_;
			std::map<const size_t, StopInfo> id_to_vertex_map_;
			std::map<const size_t, const EdgeInfo> id_to_edge_map_;
		};
	}//size_t_wrapper
}