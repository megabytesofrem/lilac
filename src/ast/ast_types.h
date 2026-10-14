#pragma once
#include <memory>
#include <string>
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

    struct Data {
        std::string value;
        std::unique_ptr<TypeInfo> childType;

        Data() = default;

        explicit Data(std::string_view value) : value(value) {}
        explicit Data(TypeInfo* childType) : childType(childType) {}
        explicit Data(std::unique_ptr<TypeInfo> childType)
            : childType(std::move(childType)) {}
    } data;

    explicit TypeInfo(Type type) : type(type), data() {}

    TypeInfo(Type type, std::string_view value) : type(type), data(value) {}
    TypeInfo(Type type, TypeInfo* childType) : type(type), data(childType) {}
    TypeInfo(Type type, std::unique_ptr<TypeInfo> childType)
        : type(type), data(std::move(childType)) {}

    TypeInfo(const TypeInfo& other) : type(other.type) {
        data.value = other.data.value;
        if (other.data.childType) {
            data.childType = std::make_unique<TypeInfo>(*other.data.childType);
        }
    }
    TypeInfo& operator=(const TypeInfo& other) {
        if (this != &other) {
            *this = TypeInfo(other);
        }
        return *this;
    }
    TypeInfo(TypeInfo&&) noexcept = default;
    TypeInfo& operator=(TypeInfo&&) noexcept = default;
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
