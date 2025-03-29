#pragma once
#include <optional>

#include "json.hpp"

namespace json_reader
{
	class JsonReader {
	public:
		JsonReader() = default;
		~JsonReader() = default;

		void ReadDocument(std::istream& input_stream = std::cin);

		std::optional<json::Node*> GetBaseRequestsNode() const;
		std::optional<json::Node*> GetStatRequestsNode() const;
		std::optional<json::Node*> GetRenderSettingsNode() const;
		std::optional<json::Node*> GetInitRouterNode() const;

	private:
		json::Document document_{ json::Node{ std::nullptr_t{} } };
	};
}//json_reader