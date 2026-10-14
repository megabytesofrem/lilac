#pragma once

#include <memory>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

#include "ast_node.h"

class LetStmt : public Node {
   private:
    std::string_view name_;
    std::unique_ptr<Node> value_;

   public:
    LetStmt(std::string_view name, std::unique_ptr<Node> value)
        : name_(name), value_(std::move(value)) {}

    std::string_view name() const { return name_; }
    const Node* value() const { return value_.get(); }
};