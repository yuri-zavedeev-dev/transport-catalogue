#include "transport_catalogue.hpp"
#include "request_handler.hpp"
#include "json_reader.hpp"
#include "transport_router.hpp"

int main() {
	using Catalogue = transport_catalogue::TransportCatalogue;

	using JSONReader = json_reader::JsonReader;
	using Configurator = transport_catalogue::configurator::json_io::DataBaseConfigurator;

	using IOHandler = transport_catalogue::io_handler::json_io::DataBaseIOHandler;

	Catalogue my_transport_catalogue{};

		JSONReader my_json_reader{};
		my_json_reader.ReadDocument();
		
		Configurator configurator{ &my_transport_catalogue };
		if (auto render_node = my_json_reader.GetRenderSettingsNode(); render_node.has_value()) {
			configurator.ReadMapRenderQuery(*render_node.value());
		}
		if (auto init_router_node = my_json_reader.GetInitRouterNode(); init_router_node.has_value()) {
			configurator.ReadInitRouterQuery(*init_router_node.value());
		}
		if (auto base_node = my_json_reader.GetBaseRequestsNode(); base_node.has_value()) {
			configurator.SetCatalogue(*base_node.value());

			if (auto stat_node = my_json_reader.GetStatRequestsNode(); stat_node.has_value()) {
				IOHandler io_handler{ &my_transport_catalogue };
				io_handler.ProcessIOQueries(*stat_node.value());
			}
		}
	return 0;
}