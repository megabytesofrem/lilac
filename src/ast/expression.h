#pragma once

#include <memory>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

#include "ast_node.h"

/**
 * Represents a literal expression in the AST.
 */
class LiteralExpr : public Node {
   public:
    using Value = std::variant<int, double, std::string_view, char, bool>;

    explicit LiteralExpr(Value value) : value(value) {}

    const Value& getValue() const { return value; }

   private:
    Value value;
};

/**
 * Represents an identifier expression in the AST.
 */
class IdentifierExpr : public Node {
   private:
    std::string_view name;

   public:
    IdentifierExpr(std::string_view name) : name(name) {}

    std::string_view getName() const { return name; }

   public:
    virtual ~IdentifierExpr() = default;
};

/**
 * Represents an array expression in the AST.
 */
class ArrayExpr : public Node {
   private:
    std::vector<NodePtr> elements;

   public:
    explicit ArrayExpr(std::vector<NodePtr> elements)
        : elements(std::move(elements)) {}

    const std::vector<NodePtr>& getElements() const { return elements; }

   public:
    virtual ~ArrayExpr() = default;
};

/**
 * Represents a procedural call in the AST.
 */
class CallExpr : public Node {
   private:
    NodePtr callee;
    std::vector<NodePtr> arguments;

   public:
    CallExpr(NodePtr callee, std::vector<NodePtr> arguments)
        : callee(std::move(callee)), arguments(std::move(arguments)) {}

    Node* getCallee() const { return callee.get(); }
    const std::vector<NodePtr>& getArguments() const { return arguments; }

   public:
    virtual ~CallExpr() = default;
};

/**
 * Represents an assignment expression in the AST.
 */
class AssignExpr : public Node {
   private:
    std::string_view name;
    NodePtr value;

   public:
    AssignExpr(std::string_view name, NodePtr value)
        : name(name), value(std::move(value)) {}

    std::string_view getName() const { return name; }
    Node* getValue() const { return value.get(); }

   public:
    virtual ~AssignExpr() = default;
};

class ExprItem : public Item {
   private:
    NodePtr expression;

   public:
    explicit ExprItem(NodePtr expression) : expression(std::move(expression)) {}

    Node* getExpression() const { return expression.get(); }

   public:
    virtual ~ExprItem() = default;
};
