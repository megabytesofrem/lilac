#pragma once
#include <string_view>
#include <vector>

#include "ast_node.h"

enum class Type {
    Int,
    Float,
    Bool,
    String,
    Array,
    Tuple,
    Function,
    Class,
    Void
};

struct Message {
    std::string_view target;
    std::string_view selector;
    std::vector<ArgumentPair<std::string_view, Node*>> keyword_args;

    Message(
        std::string_view target, std::string_view selector,
        const std::vector<ArgumentPair<std::string_view, Node*>>& keyword_args)
        : target(target), selector(selector), keyword_args(keyword_args) {}

    ~Message() = default;
};

struct MessageHandler {
    Message message;
    Node* handler;

    MessageHandler(Message message, Node* handler)
        : message(message), handler(handler) {}
};
