#include "json_reader.hpp"

namespace json_reader
{
	void JsonReader::ReadDocument(std::istream& input_stream) {
		document_ = json::Load(input_stream);
	}

	std::optional<json::Node*> JsonReader::GetBaseRequestsNode() const {
		if (auto it = document_.GetRoot().AsDict().find("base_requests"); it != document_.GetRoot().AsDict().end()) {
			return &const_cast<json::Node&>(it->second);
		}
		return std::nullopt;
	}

	std::optional<json::Node*> JsonReader::GetStatRequestsNode() const {
		if (auto it = document_.GetRoot().AsDict().find("stat_requests"); it != document_.GetRoot().AsDict().end()) {
			return &const_cast<json::Node&>(it->second);
		}
		return std::nullopt;
	}

	std::optional<json::Node*> JsonReader::GetRenderSettingsNode() const {
		if (auto it = document_.GetRoot().AsDict().find("render_settings"); it != document_.GetRoot().AsDict().end()) {
			return &const_cast<json::Node&>(it->second);
		}
		return std::nullopt;
	}

	std::optional<json::Node*> JsonReader::GetInitRouterNode() const {
		if (auto it = document_.GetRoot().AsDict().find("routing_settings"); it != document_.GetRoot().AsDict().end()) {
			return &const_cast<json::Node&>(it->second);
		}
		return std::nullopt;
	}
}