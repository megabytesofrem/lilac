#pragma once

#include <string_view>
#include <vector>

#include "ast_node.h"
#include "literal_type.h"

/**
 * Represents a literal expression in the AST.
 */
template <typename T>
class LiteralExpr : public Node {
   private:
    LiteralType type;
    T value;

   public:
    LiteralExpr(LiteralType type, T value) : type(type), value(value) {}

   public:
    virtual ~LiteralExpr() = default;
};

/**
 * Represents an identifier expression in the AST.
 */
class IdentifierExpr : public Node {
   private:
    std::string_view name;

   public:
    IdentifierExpr(std::string_view name) : name(name) {}

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

   public:
    virtual ~AssignExpr() = default;
};