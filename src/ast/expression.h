#pragma once

#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

#include "ast_node.h"

/**
 * A literal expression in the AST.
 */
class LiteralExpr : public ASTNode {
   public:
    using Value = std::variant<int, double, std::string_view, char, bool>;

    explicit LiteralExpr(Value value) : value(value) {}

    const Value& getValue() const { return value; }

   private:
    Value value;
};

/**
 * An identifier expression in the AST.
 */
class IdentifierExpr : public ASTNode {
   private:
    std::string _name;

   public:
    explicit IdentifierExpr(std::string_view name) : _name(name) {}

    std::string_view getName() const { return _name; }

   public:
    virtual ~IdentifierExpr() = default;
};

/**
 * An array expression in the AST.
 */
class ArrayExpr : public ASTNode {
   private:
    std::vector<NodePtr> _elements;

   public:
    explicit ArrayExpr(std::vector<NodePtr> elements)
        : _elements(std::move(elements)) {}

    const std::vector<NodePtr>& getElements() const { return _elements; }

   public:
    virtual ~ArrayExpr() = default;
};

/**
 * A procedural call in the AST.
 */
class CallExpr : public ASTNode {
   private:
    NodePtr _callee;
    std::vector<NodePtr> _arguments;

   public:
    CallExpr(NodePtr callee, std::vector<NodePtr> arguments)
        : _callee(std::move(callee)), _arguments(std::move(arguments)) {}

    ASTNode* getCallee() const { return _callee.get(); }
    const std::vector<NodePtr>& getArguments() const { return _arguments; }

   public:
    virtual ~CallExpr() = default;
};

/**
 * A binary expression in the AST.
 */
class BinExpr : public ASTNode {
   private:
    std::string _operatorSymbol;
    NodePtr _left;
    NodePtr _right;

   public:
    BinExpr(std::string operatorSymbol, NodePtr left, NodePtr right)
        : _operatorSymbol(std::move(operatorSymbol)),
          _left(std::move(left)),
          _right(std::move(right)) {}

    std::string_view getOperator() const { return _operatorSymbol; }
    ASTNode* getLeft() const { return _left.get(); }
    ASTNode* getRight() const { return _right.get(); }
};

/**
 * A unary expression in the AST.
 */
class UnaryExpr : public ASTNode {
   private:
    std::string _operatorSymbol;
    NodePtr _operand;

   public:
    UnaryExpr(std::string operatorSymbol, NodePtr operand)
        : _operatorSymbol(std::move(operatorSymbol)),
          _operand(std::move(operand)) {}

    std::string_view getOperator() const { return _operatorSymbol; }
    ASTNode* getOperand() const { return _operand.get(); }
};

/**
 * An assignment expression in the AST.
 */
class AssignExpr : public ASTNode {
   private:
    std::string _name;
    NodePtr _value;

   public:
    AssignExpr(std::string_view name, NodePtr value)
        : _name(name), _value(std::move(value)) {}

    std::string_view getName() const { return _name; }
    ASTNode* getValue() const { return _value.get(); }

   public:
    virtual ~AssignExpr() = default;
};

class ExprItem : public ASTItem {
   private:
    NodePtr _expression;

   public:
    explicit ExprItem(NodePtr expression)
        : _expression(std::move(expression)) {}

    ASTNode* getExpression() const { return _expression.get(); }

   public:
    virtual ~ExprItem() = default;
};
