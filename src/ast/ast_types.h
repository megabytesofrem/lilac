#pragma once
#include <string_view>
#include <utility>
#include <vector>

#include "ast_node.h"

enum class LiteralType { Integer, Float, String, Char };

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
    std::vector<ArgumentPair<std::string_view, NodePtr>> keyword_args;

    Message(std::string_view target, std::string_view selector,
            std::vector<ArgumentPair<std::string_view, NodePtr>> keyword_args)
        : target(target),
          selector(selector),
          keyword_args(std::move(keyword_args)) {}
};

struct MessageHandler {
    Message message;
    NodePtr handler;

    MessageHandler(Message message, NodePtr handler)
        : message(std::move(message)), handler(std::move(handler)) {}
};
