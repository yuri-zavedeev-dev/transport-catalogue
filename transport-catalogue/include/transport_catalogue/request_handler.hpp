#pragma once

#include <algorithm>
#include "cassert"
#include <forward_list>
#include <iomanip>
#include <iostream>
#include <queue>
#include <string>
#include <sstream>
#include <variant>
#include <vector>

#include "geo.hpp"
#include "transport_catalogue.hpp"
#include "map_renderer.hpp"
#include "json_builder.hpp"

namespace transport_catalogue
{
	namespace configurator
	{
		enum class QueryType {
			StopCreate,
			RouteCreate,
			DistanceSet,
			MapRender,
			InitRouter
		};

		struct StringViewPairHasher {
			auto operator() (std::pair<std::string_view, std::string_view> pair) const -> size_t {
				return std::hash<std::string_view>{}(pair.first) * 47
					+ std::hash<std::string_view>{}(pair.second) * 47 * 47;
			}
		};

		struct StopCreateQueryContent {
			std::string_view name;
			geo::Coordinates location;
		};

		struct RouteCreateQueryContent {
			std::string_view name;
			std::vector<std::string_view> stops_names;
			bool is_round_trip;
		};

		struct DistanceSetQueryContent {
			std::unordered_map<
				std::pair<std::string_view, std::string_view>
				, unsigned long, StringViewPairHasher
			> distances;
		};

		struct InitRouterQueryContent {
			unsigned int bus_wait_time;
			unsigned int bus_velocity;
		};

		struct Query {
			QueryType type;
			std::variant<StopCreateQueryContent
						, RouteCreateQueryContent
						, DistanceSetQueryContent
						, svg_renderer::RenderSettings
						, InitRouterQueryContent
			> content;
		};

		class QueryPtrCompare {
		public:
			bool operator() (const Query* lhs, const Query* rhs) const {
				//MapRender - highest priority
				//StopCreate - high priority
				//DistanceSet - middle priority
				//RouteCreate - low priority
				//InitRouter - lowest priority
				if (lhs->type == rhs->type) {
					return false;
				}
				if (lhs->type == QueryType::MapRender) {
					return false;
				}
				if (rhs->type == QueryType::MapRender) {
					return true;
				}
				if (lhs->type == QueryType::StopCreate) {
					return false;
				}
				if (rhs->type == QueryType::StopCreate) {
					return true;
				}
				if (lhs->type == QueryType::DistanceSet) {
					return false;
				}
				if (rhs->type == QueryType::DistanceSet) {
					return true;
				}
				if (rhs->type == QueryType::InitRouter) {
					return false;
				}
				return true;
			}
		};

		class IInputReader {
		public:
			virtual ~IInputReader() = default;

		protected:
			std::priority_queue<Query*, std::vector<Query*>, QueryPtrCompare>* query_ptr_queue_;
			std::unordered_set<std::string> unique_strings;

			std::deque<Query>* queries_;
		};

		class IDataBaseConfigurator {
		public:
			virtual ~IDataBaseConfigurator() = default;

		protected:
			void ExecuteQueries();
			void ExecuteQuery(Query& query);

			void FindRouteStops(
				std::vector<std::string_view>& stops_names_ref
				, svg_renderer::RouteData& route_data
			);

			TransportCatalogue* catalogue_;
			std::priority_queue<Query*, std::vector<Query*>, QueryPtrCompare> query_ptr_queue_;
			std::deque<Query> queries_;
		};

		namespace json_io
		{
			class InputReader : public IInputReader{
			public:
				InputReader(
					std::priority_queue<Query*,std::vector<Query*>,QueryPtrCompare>* query_ref_queue
					, std::deque<Query>* queries
				);

				size_t ReadQueries(const json::Node& node);
				void ProcessMapRenderQuery(const json::Node& node);
				void ProcessInitRouterQuery(const json::Node& node);

			private:
				void ProcessStopQuery(const json::Node& node);
				void ProcessRouteQuery(const json::Node& node);

				std::vector<std::string_view> MakeRouteCircle(std::vector<std::string_view>&& stops);

				svg_renderer::Color ParseColorFromJSON(const json::Node& node);
			};

			class DataBaseConfigurator : public IDataBaseConfigurator {
			public:
				DataBaseConfigurator() = delete;
				DataBaseConfigurator(TransportCatalogue* catalogue);
				~DataBaseConfigurator() = default;

				void SetCatalogue(const json::Node& node_ref);
				void ReadMapRenderQuery(const json::Node& node);
				void ReadInitRouterQuery(const json::Node& node);

			private:
				size_t GetQueries(const json::Node& node_ref);
				InputReader input_reader_{ &query_ptr_queue_, &queries_ };
			};
		}
	}//configurator

	namespace io_handler
	{
		enum class QueryType {
			RouteInfo,
			StopInfo,
			DrawMap,
			BuildRoute
		};

		struct StopInfoQueryContent {
			std::string name;
		};

		struct RouteInfoQueryContent {
			std::string name;
		};

		struct BuildRouteQueryContent {
			std::string from;
			std::string to;
		};

		struct Query {
			int id;
			QueryType type;
			std::variant<
				StopInfoQueryContent
				, RouteInfoQueryContent
				, std::monostate
				, BuildRouteQueryContent
			> content;
		};

		class IInputReader {
		public:
			virtual ~IInputReader() = default;

		protected:
			std::queue<Query>* query_queue_;
		};

		class IDataBaseIOHandler {
		public:
			virtual ~IDataBaseIOHandler() = default;

		protected:
			void ExecuteQueries();
			virtual void ExecuteQuery(Query& query) = 0;

			TransportCatalogue* catalogue_;
			std::queue<Query> query_queue_;
		};

		namespace json_io
		{
			class InputReader : public IInputReader{
			public:
				InputReader(std::queue<Query>* query_queue);

				size_t ReadQueries(const json::Node& node);

				void ProcessDrawMapQuery(const json::Node& node);

			private:
				void ProcessRouteGetInfoQuery(const json::Node& node);
				void ProcessStopGetInfoQuery(const json::Node& node);
				void ProcessBuildRouteQuery(const json::Node& node);
			};

			class DataBaseIOHandler : public IDataBaseIOHandler {
			public:
				DataBaseIOHandler() = delete;
				DataBaseIOHandler(TransportCatalogue* catalogue);
				~DataBaseIOHandler() = default;

				void ProcessIOQueries(const json::Node& node_ref);

			private:
				size_t GetQueries(const json::Node& node_ref);
				void ExecuteQuery(Query& query) override;
				void PrintAnswers(std::ostream& output_stream = std::cout);

				void PrintStopInfo(const details::StopInfo& info, const int id);
				void PrintRouteInfo(const details::RouteInfo& info, const int id);
				void PrintRouterBuildedInfo(
					const std::optional<const transport_router::RouteInfo* const>&& info
					, const int id
				);

				json::Array answer_{};
				InputReader input_reader_{ &query_queue_ };
			};
		}//json_io
	}//io_handler
}//transport_catalogue
