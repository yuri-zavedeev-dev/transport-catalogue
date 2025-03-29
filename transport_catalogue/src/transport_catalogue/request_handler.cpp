#include "request_handler.hpp"

namespace transport_catalogue
{
	svg_renderer::Renderer renderer{};

	namespace configurator
	{
		void IDataBaseConfigurator::ExecuteQueries() {
			while (query_ptr_queue_.size() != 0) {
				ExecuteQuery(const_cast<Query&>(*query_ptr_queue_.top()));
				query_ptr_queue_.pop();
			}
		}

		namespace json_io
		{
			InputReader::InputReader(
				std::priority_queue<Query*,std::vector<Query*>
				, QueryPtrCompare>* query_ref_queue
				, std::deque<Query>* queries)
			{
				query_ptr_queue_ = query_ref_queue; //can't be represented with init-list, because is not base of InputReader
				queries_ = queries; //can't be represented with init-list, because is not base of InputReader
			}

			size_t InputReader::ReadQueries(const json::Node& node) {
				size_t queries_count{ node.AsArray().size() };

				for (const json::Node& query_node : node.AsArray()) {
					auto type_it = query_node.AsDict().find("type");
					if (type_it->second.AsString() == "Stop") {
						ProcessStopQuery(query_node);
					}
					else if(type_it->second.AsString() == "Bus"){
						ProcessRouteQuery(query_node);
					}
					else {
						std::ostringstream oss;
						json::Print(json::Document{ node }, oss);
						std::string error_message = {
							"configurator::InputReader::ReadQueries(const json::Node&): No such query type!\nNode:\n"
							+ oss.str()
						};

						throw std::logic_error{ error_message };
					}
				}
				
				return queries_count;
			}

			void InputReader::ProcessStopQuery(const json::Node& node) {
				std::unordered_map<
					std::pair<std::string_view, std::string_view>
					, unsigned long, StringViewPairHasher
				> distances;

				std::string_view stop_name_sv = *(unique_strings.insert(node.AsDict().find("name")->second.AsString()).first);
				for (auto& [dst_stop_name, distance] : node.AsDict().find("road_distances")->second.AsDict()) {
					std::string_view dst_stop_name_sv = *(unique_strings.insert(dst_stop_name).first);
					distances[{ stop_name_sv, dst_stop_name_sv }] = static_cast<unsigned long>(distance.AsInt());
				}

				if (!distances.empty())
				{
					queries_->push_back(std::move(Query{
							.type = QueryType::DistanceSet
							, .content = DistanceSetQueryContent{
								.distances = std::move(distances)
							}
						}));

					query_ptr_queue_->push(&queries_->back());
				}

				queries_->push_back(std::move(Query{
						.type = QueryType::StopCreate
						, .content = StopCreateQueryContent{
							.name = std::move(stop_name_sv)
							, .location = {
								node.AsDict().find("latitude")->second.AsDouble()
								, node.AsDict().find("longitude")->second.AsDouble()
							}
						}
					}));

				query_ptr_queue_->push(&queries_->back());
			}

			void InputReader::ProcessRouteQuery(const json::Node& node) {
				std::vector<std::string_view> stops;
				std::string_view route_name_sv = *(unique_strings.insert(node.AsDict().find("name")->second.AsString()).first);

				for (const json::Node& stop : node.AsDict().find("stops")->second.AsArray()) {
					std::string_view stop_name_sv = *(unique_strings.insert(stop.AsString()).first);
					stops.push_back(stop_name_sv);
				}

				bool is_round_trip{ true };
				if (node.AsDict().find("is_roundtrip")->second.AsBool() == false) {
					stops = std::move(MakeRouteCircle(std::move(stops)));
					is_round_trip = false;
				}

				queries_->push_back(std::move(Query{
						.type = QueryType::RouteCreate
						, .content = RouteCreateQueryContent{
							.name = std::move(route_name_sv)
							, .stops_names = std::move(stops)
							, .is_round_trip = is_round_trip
						}
					}));

				query_ptr_queue_->push(&queries_->back());
			}

			std::vector<std::string_view> InputReader::MakeRouteCircle(std::vector<std::string_view>&& stops) {
				std::vector<std::string_view> stops_copy{};
				stops_copy.reserve(stops.size());

				for (auto it{ stops.begin() }; it != std::prev(stops.end()); it++) {
					stops_copy.emplace_back(*it);
				}

				std::reverse(stops_copy.begin(), stops_copy.end());
				stops.reserve(stops.size() + stops_copy.size());

				for (auto it{ stops_copy.begin() }; it != stops_copy.end(); it++) {
					stops.emplace_back(std::move(*it));
				}

				return std::move(stops);
			}

			void InputReader::ProcessInitRouterQuery(const json::Node& node) {
				unsigned int bus_wait_time;
				unsigned int bus_velocity;

				const json::Dict& params = node.AsDict();
				bus_wait_time = params.at("bus_wait_time").AsInt();
				bus_velocity = params.at("bus_velocity").AsInt();

				queries_->push_back(std::move(Query{
						.type = QueryType::InitRouter
						, .content = InitRouterQueryContent{
							.bus_wait_time = bus_wait_time
							, .bus_velocity = bus_velocity
						}
					}));

				query_ptr_queue_->push(&queries_->back());
			}

			svg_renderer::Color InputReader::ParseColorFromJSON(const json::Node& node) {
				if (node.IsString()) {
					return { node.AsString() };
				}
				if (node.IsArray()) {
					if (node.AsArray().size() == 3) {
						const json::Array& color_array = node.AsArray();
						return std::tuple<int, int, int>{ color_array[0].AsInt()
														, color_array[1].AsInt()
														, color_array[2].AsInt()
						};
					}
					else if (node.AsArray().size() == 4) {
						const json::Array& color_array = node.AsArray();
						return std::tuple<int, int, int, double>{ color_array[0].AsInt()
																, color_array[1].AsInt()
																, color_array[2].AsInt()
																, color_array[3].AsDouble()
						};
					}
				}
				throw std::logic_error{"svg_renderer::Color ParseColorFromJSON(const json::Node&): Unknown color type!"};
			}
			void InputReader::ProcessMapRenderQuery(const json::Node& node) {
				svg_renderer::RenderSettings settings;
				settings.width = node.AsDict().find("width")->second.AsDouble();
				settings.height = node.AsDict().find("height")->second.AsDouble();

				settings.padding = node.AsDict().find("padding")->second.AsDouble();

				settings.line_width = node.AsDict().find("line_width")->second.AsDouble();
				settings.stop_radius = node.AsDict().find("stop_radius")->second.AsDouble();

				settings.bus_label_font_size = node.AsDict().find("bus_label_font_size")->second.AsInt();
				settings.bus_label_offset = { node.AsDict().find("bus_label_offset")->second.AsArray()[0].AsDouble()
											, node.AsDict().find("bus_label_offset")->second.AsArray()[1].AsDouble() };

				settings.stop_label_font_size = node.AsDict().find("stop_label_font_size")->second.AsInt();
				settings.stop_label_offset = { node.AsDict().find("stop_label_offset")->second.AsArray()[0].AsDouble()
											, node.AsDict().find("stop_label_offset")->second.AsArray()[1].AsDouble() };

				settings.underlayer_color = ParseColorFromJSON(node.AsDict().find("underlayer_color")->second);
				settings.underlayer_width = node.AsDict().find("underlayer_width")->second.AsDouble();

				for (const json::Node& node : node.AsDict().find("color_palette")->second.AsArray()) {
					settings.palette.push_back(ParseColorFromJSON(node));
				}

				queries_->push_back(std::move(Query{
					.type = QueryType::MapRender
					, .content = std::move(settings)
					}));

				query_ptr_queue_->push(&queries_->back());
			}

			DataBaseConfigurator::DataBaseConfigurator(TransportCatalogue* catalogue) {
				catalogue_ = std::move(catalogue); 	//Can't be represented with init-list
													//, because is not base of DataBaseConfigurator.
													//The other fields are already initialized.
			}

			void DataBaseConfigurator::SetCatalogue(const json::Node& node_ref) {
				GetQueries(node_ref);
				ExecuteQueries();
			}

			size_t DataBaseConfigurator::GetQueries(const json::Node& node_ref) {
				return input_reader_.ReadQueries(node_ref);
			}

			void DataBaseConfigurator::ReadMapRenderQuery(const json::Node& node) {
				input_reader_.ProcessMapRenderQuery(node);
			}

			void DataBaseConfigurator::ReadInitRouterQuery(const json::Node& node) {
				input_reader_.ProcessInitRouterQuery(node);
			}
		}

		void IDataBaseConfigurator::FindRouteStops(
			std::vector<std::string_view>& stops_names_ref
			, svg_renderer::RouteData& route_data
		) {
			for (const std::string_view stop_name_sv : stops_names_ref) {
				auto it = std::find_if(queries_.begin(), queries_.end(), [&stop_name_sv](const Query& q)->bool {
					if (q.type != QueryType::StopCreate) {
						return false;
					}
					return stop_name_sv == std::get<StopCreateQueryContent>(q.content).name;
					});

				svg_renderer::StopData stop_data{
					std::get<StopCreateQueryContent>(it->content).name
					, std::get<StopCreateQueryContent>(it->content).location
				};

				route_data.stops.push_back(std::move(stop_data));
			}
		}

		void IDataBaseConfigurator::ExecuteQuery(Query& query) {
			switch (query.type) {
			case QueryType::StopCreate:
			{
				catalogue_->AddStop(std::move(static_cast<std::string>(std::get<StopCreateQueryContent>(query.content).name))
					, std::move(std::get<StopCreateQueryContent>(query.content).location)
				);
				break;
			}
			case QueryType::RouteCreate:
			{
				catalogue_->AddRoute(std::move(static_cast<std::string>(std::get<RouteCreateQueryContent>(query.content).name))
					, std::move(std::get<RouteCreateQueryContent>(query.content).stops_names)
				);
				break;
			}
			case QueryType::DistanceSet:
			{
				for (auto& [names, distance] : std::get<DistanceSetQueryContent>(query.content).distances) {
					catalogue_->SetDistanceBetweenStops(
						catalogue_->GetStopPtr(names.first)
						, catalogue_->GetStopPtr(names.second)
						, distance
					);
				}
				break;
			}
			case QueryType::InitRouter:
			{
				catalogue_->InitRouter({
					.bus_wait_time = std::get<InitRouterQueryContent>(query.content).bus_wait_time
					, .bus_velocity = std::get<InitRouterQueryContent>(query.content).bus_velocity
					, .catalogue = catalogue_
					}
				);
			}
				break;
			case QueryType::MapRender:
			{				
				for (Query& query_local : queries_) {
					if (query_local.type == QueryType::RouteCreate) {
						svg_renderer::RouteData route_data{  };
						route_data.name = std::get<RouteCreateQueryContent>(query_local.content).name;
						route_data.is_round_trip = std::get<RouteCreateQueryContent>(query_local.content).is_round_trip;
						std::vector<std::string_view>& stops_names_ref{
							std::get<RouteCreateQueryContent>(query_local.content).stops_names
						};

						FindRouteStops(stops_names_ref, route_data);

						renderer.AddRoute(std::move(route_data));
					}
				}

				renderer.SetSettings(std::move(std::get<svg_renderer::RenderSettings>(query.content)));
				break;
			}
			default:
				throw std::logic_error{ "DataBaseConfigurator::ExecuteQuery: Unknown query type!" };
			}
		}
	}//configurator

	namespace io_handler
	{
		void IDataBaseIOHandler::ExecuteQueries() {
			while (query_queue_.size() != 0) {
				ExecuteQuery(const_cast<Query&>(query_queue_.front()));
				query_queue_.pop();
			}
		}
		
		namespace json_io
		{
			InputReader::InputReader(std::queue<Query>* query_queue) {
				query_queue_ = query_queue; //can't be represented with init-list, because is not base of InputReader
			}

			size_t InputReader::ReadQueries(const json::Node& node) {
				size_t queries_count{ node.AsArray().size() };

				for (const json::Node& query_node : node.AsArray()) {
					auto type_it = query_node.AsDict().find("type");
					if (type_it->second.AsString() == "Stop") {
						ProcessStopGetInfoQuery(query_node);
					}
					else if (type_it->second.AsString() == "Bus") {
						ProcessRouteGetInfoQuery(query_node);
					}
					else if (type_it->second.AsString() == "Map") {
						ProcessDrawMapQuery(query_node);
					}
					else if (type_it->second.AsString() == "Route") {
						ProcessBuildRouteQuery(query_node);
					}
					else {
						std::ostringstream oss;
						json::Print(json::Document{ node }, oss);
						std::string error_message = {
							"io_handler::InputReader::ReadQueries(const json::Node&): No such query type!\nNode:\n"
							+ oss.str()
						};

						throw std::logic_error{ error_message };
					}
				}

				return queries_count;
			}

			void InputReader::ProcessDrawMapQuery(const json::Node& node)
			{
				Query query{ 
					.id = node.AsDict().find("id")->second.AsInt()
					, .type = QueryType::DrawMap
					, .content = std::monostate{}
				};
				query_queue_->push(std::move(query));
			}

			void InputReader::ProcessRouteGetInfoQuery(const json::Node& node) {
				Query query{ 
					.id = node.AsDict().find("id")->second.AsInt()
					, .type = QueryType::RouteInfo
					, .content = RouteInfoQueryContent{ .name = node.AsDict().find("name")->second.AsString() }
				};
				query_queue_->push(std::move(query));
			}

			void InputReader::ProcessStopGetInfoQuery(const json::Node& node) {
				Query query{ 
					.id = node.AsDict().find("id")->second.AsInt()
					, .type = QueryType::StopInfo
					, .content = StopInfoQueryContent{ .name = node.AsDict().find("name")->second.AsString() }
				};
				query_queue_->push(std::move(query));
			}

			void InputReader::ProcessBuildRouteQuery(const json::Node& node) {
				Query query{
					.id = node.AsDict().find("id")->second.AsInt()
					, .type = QueryType::BuildRoute
					, .content = BuildRouteQueryContent{
							.from = node.AsDict().find("from")->second.AsString()
							, .to = node.AsDict().find("to")->second.AsString()
						}
				};
				query_queue_->push(std::move(query));
			}

			void DataBaseIOHandler::PrintStopInfo(const details::StopInfo& info, const int id) {
				using namespace std::literals::string_literals;
				json::Array routes;
				if (!info.routes->empty())
				{
					std::vector<std::string_view> route_names;
					for (details::Route* route_ptr : *info.routes) {
						route_names.push_back(route_ptr->name);
					}

					std::sort(route_names.begin(), route_names.end(), [](std::string_view lhs, std::string_view rhs) {
						return std::lexicographical_compare(lhs.begin(), lhs.end()
							, rhs.begin(), rhs.end()
						);
					});

					json::Array routes_array;
					for (std::string_view route_name_sv : route_names) {
						routes_array.push_back(static_cast<std::string>(route_name_sv));
					}

					answer_.push_back(json::Builder{}.StartDict()
							.Key("request_id"s).Value(id)
							.Key("buses"s).Value(routes_array)
						.EndDict().Build()
					);
				}
				else {
					answer_.push_back(json::Builder{}.StartDict()
							.Key("request_id"s).Value(id)
							.Key("buses"s).Value(json::Array{})
						.EndDict().Build()
					);
				}
			}

			void DataBaseIOHandler::PrintRouteInfo(const details::RouteInfo& info, const int id) {
				using namespace std::literals::string_literals;
				answer_.push_back(json::Builder{}.StartDict()
						.Key("request_id"s).Value(id)
						.Key("route_length"s).Value(static_cast<int>(info.distance_total))
						.Key("stop_count"s).Value(static_cast<int>(info.stops_count))
						.Key("unique_stop_count"s).Value(static_cast<int>(info.unique_stops_count))
						.Key("curvature"s).Value(static_cast<double>(info.curvature))
					.EndDict().Build()
				);
			}

			void DataBaseIOHandler::PrintRouterBuildedInfo(
				const std::optional<const transport_router::RouteInfo* const>&& info
				, const int id
			) {
				using namespace std::literals::string_literals;
				if (!info) {
					answer_.push_back(json::Builder{}.StartDict()
						.Key("request_id"s).Value(id)
						.Key("error_message"s).Value("not found"s)
						.EndDict().Build()
					);
					return;
				}

				json::Array items;
				for (const auto& item : info.value()->items) {
					if (const auto wait_item = std::get_if<transport_router::WaitItem>(&item.content)) {
						items.push_back(
							json::Dict{
								{"type", json::Node("Wait"s)}
								, {"stop_name", json::Node(static_cast<std::string>(wait_item->stop_name))}
								, {"time", json::Node(static_cast<int>(wait_item->time))}
							}
						);
					}

					if (const auto bus_item = std::get_if<transport_router::BusItem>(&item.content)) {
						items.push_back(
							json::Dict{
								{"type", json::Node("Bus"s)}
								, {"bus", json::Node(static_cast<std::string>(bus_item->bus))}
								, {"span_count", json::Node(static_cast<int>(bus_item->span_count))}
								, {"time", json::Node(bus_item->time)}
							}
						);
					}
				}

				answer_.push_back(json::Builder{}.StartDict()
					.Key("request_id"s).Value(id)
					.Key("total_time"s).Value(info.value()->total_time)
					.Key("items"s).Value(items)
					.EndDict().Build()
				);
			}

			DataBaseIOHandler::DataBaseIOHandler(TransportCatalogue* catalogue) {
				catalogue_ = catalogue;//can't be represented with init-list, because is not base of DataBaseIOHandler
				//the other fields are already initialized
			}

			void DataBaseIOHandler::ProcessIOQueries(const json::Node& node_ref) {
				GetQueries(node_ref);
				ExecuteQueries();
				PrintAnswers();
			}

			size_t DataBaseIOHandler::GetQueries(const json::Node& node_ref) {
				return input_reader_.ReadQueries(node_ref);
			}

			void DataBaseIOHandler::ExecuteQuery(Query& query) {
				using namespace std::literals::string_literals;
				switch (query.type) {
				case QueryType::StopInfo:
					try {
						PrintStopInfo(catalogue_->GetStopInfo(std::get<StopInfoQueryContent>(query.content).name), query.id);
					}
					catch (std::logic_error&) {
						answer_.push_back(json::Builder{}.StartDict()
								.Key("request_id"s).Value(query.id)
								.Key("error_message"s).Value("not found"s)
							.EndDict().Build()
						);
					}
					break;
				case QueryType::RouteInfo:
					try {
						PrintRouteInfo(catalogue_->GetRouteInfo(std::get<RouteInfoQueryContent>(query.content).name), query.id);
					}
					catch (std::logic_error&) {
						answer_.push_back(json::Builder{}.StartDict()
							.Key("request_id"s).Value(query.id)
							.Key("error_message"s).Value("not found"s)
							.EndDict().Build()
						);
					}
					break;
				case QueryType::BuildRoute:
				{
					auto route_info = catalogue_->BuildRoute(
						std::get<BuildRouteQueryContent>(query.content).from,
						std::get<BuildRouteQueryContent>(query.content).to
					);
					PrintRouterBuildedInfo(std::move(route_info), query.id);
				}
					break;
				case QueryType::DrawMap:
				{
					std::ostringstream oss{};
					renderer.Render(oss);
					answer_.push_back(json::Builder{}.StartDict()
						.Key("request_id"s).Value(query.id)
						.Key("map"s).Value(oss.str())
						.EndDict().Build()
					);
					break;
				}
				default:
					throw std::logic_error{ "DataBaseIOHandler::ExecuteQuery: Unknown query type!" };
				}
			}

			void DataBaseIOHandler::PrintAnswers(std::ostream& output_stream) {
				if (answer_.empty()) {
					return;
				}
				json::Print(json::Document{ answer_ }, output_stream);
			}
		}//json_io
	}//io_handler
}//transport_catalogue
