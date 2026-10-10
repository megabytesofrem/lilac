#pragma once

#include <string_view>
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
    std::vector<Node*> elements;

   public:
    ArrayExpr(const std::vector<Node*>& elements) : elements(elements) {}

    const std::vector<Node*>& getElements() const { return elements; }

   public:
    virtual ~ArrayExpr() = default;
};

/**
 * Represents a procedural call in the AST.
 */
class CallExpr : public Node {
   private:
    Node* callee;
    std::vector<Node*> arguments;

   public:
    CallExpr(Node* callee, const std::vector<Node*>& arguments)
        : callee(callee), arguments(arguments) {}

    Node* getCallee() const { return callee; }
    const std::vector<Node*>& getArguments() const { return arguments; }

   public:
    virtual ~CallExpr() = default;
};

/**
 * Represents a let expression in the AST.
 */
class LetExpr : public Node {
   private:
    std::string_view name;
    Node* value;
    Node* body;

   public:
    LetExpr(std::string_view name, Node* value, Node* body)
        : name(name), value(value), body(body) {}

    std::string_view getName() const { return name; }
    Node* getValue() const { return value; }
    Node* getBody() const { return body; }

   public:
    virtual ~LetExpr() = default;
};

/**
 * Represents an assignment expression in the AST.
 */
class AssignExpr : public Node {
   private:
    std::string_view name;
    Node* value;

   public:
    AssignExpr(std::string_view name, Node* value) : name(name), value(value) {}

    std::string_view getName() const { return name; }
    Node* getValue() const { return value; }

   public:
    virtual ~AssignExpr() = default;
};

class ExprItem : public Item {
   private:
    Node* expression;

   public:
    ExprItem(Node* expression) : expression(expression) {}

    Node* getExpression() const { return expression; }

   public:
    virtual ~ExprItem() = default;
};
