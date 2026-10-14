#pragma once
#include <string_view>
#include <utility>
#include <vector>

#include "ast_node.h"

enum class LiteralType { Integer, Float, String, Char };

enum class Type {
    // User-defined type
    User,

    Pointer, /* Raw pointer */
    ARC,     /* ARC value */
    I16,
    I32,
    I64,
    U16,
    U32,
    U64,
    F32,
    F64,
    Bool,
    String,
    Void
};

struct TypeInfo {
    Type type;

    union Data {
        std::string_view value; /* String value for the type*/
        TypeInfo* childType;    /* Child type info, if any */

        constexpr Data() : childType(nullptr) {}

        constexpr explicit Data(std::string_view value) : value(value) {}
        constexpr explicit Data(TypeInfo* childType) : childType(childType) {}
        constexpr explicit Data(std::unique_ptr<TypeInfo> childType)
            : childType(childType.release()) {}
    } data;

    explicit TypeInfo(Type type) : type(type), data() {}

    TypeInfo(Type type, std::string_view value) : type(type), data(value) {}
    TypeInfo(Type type, TypeInfo* childType) : type(type), data(childType) {}
    TypeInfo(Type type, std::unique_ptr<TypeInfo> childType)
        : type(type), data(std::move(childType)) {}
};

struct Message {
    std::string_view target;
    std::string_view selector;
    std::vector<ArgumentPair<std::string_view, NodePtr>> keyword_args;

    Message(std::string_view target,
            std::string_view selector,
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
