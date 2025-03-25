#include "json_builder.hpp"

namespace json
{
	UnfinishedNode::UnfinishedNode(Node::Value value)
		: content{ value }
	{}

	UnfinishedNode::UnfinishedNode(KeyValue key_value)
		: content{ key_value }
	{}

	UnfinishedNode::UnfinishedNode(UnfinishedNode::Bracket bracket)
		: content{ bracket }
	{}

	Builder::KeyContext Builder::Key(std::string key) {
		ThrowIfFinished("Key");
		if (current_state_ != State::ExpectingEndOfDict) {
			throw std::logic_error{ "Builder::Key(std::string): Can't add Node:\n\tKey was not expected!" };
		}

		unfinished_nodes_ptrs_stack_.push(
			std::make_unique<UnfinishedNode>(
				UnfinishedNode{ UnfinishedNode::KeyValue{std::move(key), std::nullopt} }
			)
		);
		current_state_ = State::ExpectingValue;
		return { *this };
	}

	Builder& Builder::Value(Node::Value value) {
		return PushValue(std::move(value));
	}

	Builder::DictContext Builder::StartDict() {
		ThrowIfFinished("StartDict");

		unfinished_nodes_ptrs_stack_.push(std::make_unique<UnfinishedNode>(UnfinishedNode::Bracket::DictStart));
		current_state_ = State::ExpectingEndOfDict;
		return { *this };
	}

	Builder& Builder::EndDict() {
		ThrowIfFinished("EndDict");

		if (current_state_ != State::ExpectingEndOfDict) {
			throw std::logic_error{ "Builder::EndDict(): unexpected end of Dict!" };
		}

		std::stack<UnfinishedNode*> key_values;
		while (!unfinished_nodes_ptrs_stack_.empty()) {
			UnfinishedNode* node_ptr = unfinished_nodes_ptrs_stack_.top().release();
			unfinished_nodes_ptrs_stack_.pop();

			//Is the Dict's start found
			if (auto it = std::get_if<UnfinishedNode::Bracket>(&node_ptr->content)) {
				if (*it == UnfinishedNode::Bracket::DictStart) {
					Dict new_dict;
					while (!key_values.empty()) {
						FillDictWithNodes(key_values, new_dict);
					}

					//Restoring the previous Node context
					RecoverContext();

					return PushValue(std::move(new_dict));
				}
			}

			key_values.push(node_ptr);
		}
		throw std::logic_error{ "Builder::EndDict(): Reaching begin of Nodes, but haven't find DictStart!" };
	}

	void Builder::FillDictWithNodes(std::stack<json::UnfinishedNode*>& key_values, json::Dict& new_dict)
	{
		UnfinishedNode* key_value_ptr = key_values.top();
		key_values.pop();

		UnfinishedNode::KeyValue key_value = std::move(std::get<UnfinishedNode::KeyValue>(key_value_ptr->content));
		new_dict.emplace(std::pair{ std::move(key_value.first), std::move(key_value.second.value()) });

		if (key_value_ptr) {
			delete key_value_ptr;
		}
	}

	Builder::ArrayContext Builder::StartArray() {
		ThrowIfFinished("StartArray");

		unfinished_nodes_ptrs_stack_.push(std::make_unique<UnfinishedNode>(UnfinishedNode::Bracket::ArrayStart));
		current_state_ = State::ExpectingEndOfArray;
		return { *this };
	}

	Builder& Builder::EndArray() {
		ThrowIfFinished("EndArray");

		if (current_state_ != State::ExpectingEndOfArray) {
			throw std::logic_error{ "Builder::EndArray(): unexpected end of Array!" };
		}

		std::stack<UnfinishedNode*> values;
		while (!unfinished_nodes_ptrs_stack_.empty()) {
			UnfinishedNode* node_ptr = unfinished_nodes_ptrs_stack_.top().release();
			unfinished_nodes_ptrs_stack_.pop();

			//Is the Array's start found
			if (auto it = std::get_if<UnfinishedNode::Bracket>(&node_ptr->content)) {
				if (*it == UnfinishedNode::Bracket::ArrayStart) {
					Array new_array;
					while (!values.empty()) {
						FillArrayWithNodes(values, new_array);
					}

					//Restoring the previous Node context
					RecoverContext();

					return PushValue(std::move(new_array));
				}
			}

			values.push(node_ptr);
		}

		throw std::logic_error{ "Builder::EndArray(): Reaching begin of Nodes, but haven't find ArrayStart!" };
	}

	void Builder::FillArrayWithNodes(std::stack<json::UnfinishedNode*>& values, json::Array& new_array)
	{
		UnfinishedNode* value_ptr = values.top();
		values.pop();

		Node::Value value = std::move(std::get<Node::Value>(value_ptr->content));
		new_array.emplace_back(std::move(value));

		if (value_ptr) {
			delete value_ptr;
		}
	}

	void Builder::RecoverContext()
	{
		if (!unfinished_nodes_ptrs_stack_.empty()) {
			std::unique_ptr<UnfinishedNode>& stack_top = unfinished_nodes_ptrs_stack_.top();
			if (std::holds_alternative<UnfinishedNode::KeyValue>(stack_top->content)) {
				//current value must be inserted in the previously declared pair  
				current_state_ = State::ExpectingEndOfDict;
			}
			else {
				//current value must be inserted in the previously declared array
				current_state_ = State::ExpectingEndOfArray;
			}
		}
		else {
			current_state_ = State::ExpectingValue;
		}
	}

	Node Builder::Build() {
		if (Empty()) {
			throw std::logic_error{ "Builder::Build(): Can't construct Node:\n\tNo value presented!" };
		}
		if (!unfinished_nodes_ptrs_stack_.empty()) {
			throw std::logic_error{ "Builder::Build(): Can't construct Node:\n\tSome uninitialized Nodes left!s" };
		}

		return root_.value();
	}

	bool Builder::Empty() const {
		return !root_.has_value();
	}

	Builder& Builder::PushValue(Node::Value&& value) {
		ThrowIfFinished("Value");

		//value - is root_
		if (unfinished_nodes_ptrs_stack_.empty()) {
			root_.emplace(std::move(value));
			current_state_ = State::Finished;
			return *this;
		}

		//Insertion in Array
		if (current_state_ == State::ExpectingEndOfArray) {
			unfinished_nodes_ptrs_stack_.push(std::make_unique<UnfinishedNode>(UnfinishedNode{ std::move(value) }));
			return *this;
		}

		//Insertion in Dict
		std::unique_ptr<UnfinishedNode>& stack_top = unfinished_nodes_ptrs_stack_.top();
		if (auto it = std::get_if<UnfinishedNode::KeyValue>(&stack_top->content)) {
			if (!it->second.has_value()) {
				it->second.emplace(std::move(value));
				current_state_ = State::ExpectingEndOfDict;
				return *this;
			}
		}

		throw std::logic_error{
			"Builder::Value(Node::Value): Can't add Node:\n\tFailed to emplace value to base Node or any stack Node!"
		};
	}

	Builder::IBuilderItemContext::IBuilderItemContext(Builder& builder)
		: builder_{ builder }
	{}

	bool Builder::IBuilderItemContext::Empty() const {
		return builder_.Empty();
	}

	//IBuilderItemContext
	Builder::KeyContext Builder::IBuilderItemContext::Key(std::string key) {
		return builder_.Key(key);
	}
	Builder& Builder::IBuilderItemContext::Value(Node::Value value) {
		return builder_.Value(value);
	}
	Builder::DictContext Builder::IBuilderItemContext::StartDict() {
		return builder_.StartDict();
	}
	Builder& Builder::IBuilderItemContext::EndDict() {
		return builder_.EndDict();
	}
	Builder::ArrayContext Builder::IBuilderItemContext::StartArray() {
		return builder_.StartArray();
	}
	Builder& Builder::IBuilderItemContext::EndArray() {
		return builder_.EndArray();
	}

	//KeyContext
	Builder::DictContext Builder::KeyContext::Value(Node::Value value) {
		return { builder_.Value(value) };
	}
	Builder::DictContext Builder::KeyContext::StartDict() {
		return builder_.StartDict();
	}
	Builder::ArrayContext Builder::KeyContext::StartArray() {
		return builder_.StartArray();
	}

	//DictContext
	Builder::KeyContext Builder::DictContext::Key(std::string key) {
		return builder_.Key(key);
	}
	Builder& Builder::DictContext::EndDict() {
		return builder_.EndDict();
	}

	//ArrayContext
	Builder::ArrayContext Builder::ArrayContext::Value(Node::Value value) {
		return { builder_.Value(value) };
	}
	Builder::DictContext Builder::ArrayContext::StartDict() {
		return builder_.StartDict();
	}
	Builder::ArrayContext Builder::ArrayContext::StartArray() {
		return builder_.StartArray();
	}
	Builder& Builder::ArrayContext::EndArray() {
		return builder_.EndArray();
	}
}