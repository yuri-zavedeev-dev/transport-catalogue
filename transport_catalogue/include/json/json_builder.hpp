#pragma once
#include <optional>
#include <stack>
#include <memory>

#include "json.hpp"

namespace json
{
	struct UnfinishedNode {
		using KeyValue = std::pair<std::string, std::optional<Node>>;
		enum class Bracket {
			ArrayStart,
			ArrayEnd,
			DictStart,
			DictEnd
		};

		UnfinishedNode(Node::Value);
		UnfinishedNode(KeyValue);
		UnfinishedNode(UnfinishedNode::Bracket);

		std::variant<Node::Value, KeyValue, Bracket> content;
	};

	class Builder {
	private:
		class KeyContext;
		class DictContext;
		class ArrayContext;

	public:
		KeyContext Key(std::string);
		Builder& Value(Node::Value);
		DictContext StartDict();
		Builder& EndDict();
		ArrayContext StartArray();
		Builder& EndArray();
		Node Build();

		bool Empty() const;

	private:
#define ThrowIfFinished(method_name) if(current_state_ == State::Finished){\
        std::string error_message = {"Builder::"};\
        error_message += method_name;\
        error_message += "(): Cant't modify JSON:\n\tThe base node already closed.";\
        throw std::logic_error{error_message}; }

		Builder& PushValue(Node::Value&&);

		void FillDictWithNodes(std::stack<json::UnfinishedNode*>& key_values, json::Dict& new_dict);
		void FillArrayWithNodes(std::stack<json::UnfinishedNode*>& values, json::Array& new_array);
		void RecoverContext();

		enum class State {
			Finished, //unable to modify, can only be constructed
			ExpectingValue, //await for a value or an opening bracket
			ExpectingEndOfArray, //await for a value or a closing bracket of array
			ExpectingEndOfDict //await for a value or a closing bracket of dictionary
		};

		std::optional<Node> root_{ std::nullopt };
		std::stack<std::unique_ptr<UnfinishedNode>> unfinished_nodes_ptrs_stack_;
		State current_state_{ State::ExpectingValue };

		class IBuilderItemContext {
		public:
			IBuilderItemContext(Builder& builder);

			KeyContext Key(std::string);
			Builder& Value(Node::Value);
			DictContext StartDict();
			Builder& EndDict();
			ArrayContext StartArray();
			Builder& EndArray();

			bool Empty() const;
		protected:
			Builder& builder_;
		};

		class KeyContext : public IBuilderItemContext {
		public:
			KeyContext(IBuilderItemContext base_ctx) : IBuilderItemContext(base_ctx) {}
			DictContext Value(Node::Value);
			DictContext StartDict();
			ArrayContext StartArray();

			KeyContext Key(std::string) = delete;
			Builder& EndDict() = delete;
			Builder& EndArray() = delete;
		};

		class DictContext : public IBuilderItemContext {
		public:
			DictContext(IBuilderItemContext base_ctx) : IBuilderItemContext(base_ctx) {}
			KeyContext Key(std::string);
			Builder& EndDict();

			Builder& Value(Node::Value) = delete;
			DictContext StartDict() = delete;
			ArrayContext StartArray() = delete;
			Builder& EndArray() = delete;
		};

		class ArrayContext : public IBuilderItemContext {
		public:
			ArrayContext(IBuilderItemContext base_ctx) : IBuilderItemContext(base_ctx) {}
			ArrayContext Value(Node::Value);
			DictContext StartDict();
			ArrayContext StartArray();
			Builder& EndArray();

			KeyContext Key(std::string) = delete;
			Builder& EndDict() = delete;
		};
	};
}