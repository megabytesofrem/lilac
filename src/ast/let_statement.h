#pragma once

#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

#include "ast_node.h"
#include "ast_types.h"

class LetStmt : public ASTNode {
   private:
    std::string _name;
    std::unique_ptr<ASTNode> _value;

    std::optional<TypeInfo> _type;

   public:
    LetStmt(std::string_view name,
            std::unique_ptr<ASTNode> value,
            std::optional<TypeInfo> type = std::nullopt)
        : _name(name), _value(std::move(value)), _type(std::move(type)) {}

    std::string_view name() const { return _name; }
    ASTNode* value() { return _value.get(); }
    const ASTNode* value() const { return _value.get(); }
    const std::optional<TypeInfo>& type() const { return _type; }
};