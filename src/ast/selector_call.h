#pragma once

#include <utility>

#include "ast_node.h"

/**
 * The arity of a selector call.
 *
 * Unary represents a call with one argument, binary represents a call with two
 * arguments and keyword is N-ary.
 */
enum class CallArity { Unary, Binary, Keyword };

class KeywordArgument {
   private:
    std::string_view keyword;
    NodePtr value;

   public:
    KeywordArgument(std::string_view keyword, NodePtr value)
        : keyword(keyword), value(std::move(value)) {}

    std::string_view getKeyword() const { return keyword; }
    Node* getValue() const { return value.get(); }
};

class UnarySelectorExpr : public Node {
   private:
    std::string_view selector;

   public:
    UnarySelectorExpr(std::string_view selector) : selector(selector) {}

    std::string_view getSelector() const { return selector; }

   public:
    virtual ~UnarySelectorExpr() = default;
};

class BinarySelectorExpr : public Node {
   private:
    std::string_view symbol;
    NodePtr value;

   public:
    BinarySelectorExpr(std::string_view symbol, NodePtr value)
        : symbol(symbol), value(std::move(value)) {}

    std::string_view getSymbol() const { return symbol; }
    Node* getValue() const { return value.get(); }

   public:
    virtual ~BinarySelectorExpr() = default;
};

/**
 * Represents a chain of unary message sends in the AST, where each link in the
 * chain is a unary message sent to the target receiver of the previous message.
 */
class UnaryChain : public Node {
   private:
    NodePtr target;
    std::vector<NodePtr> elements;
    CallArity arity;

   public:
    UnaryChain(NodePtr target, std::vector<NodePtr> elements, CallArity arity)
        : target(std::move(target)),
          elements(std::move(elements)),
          arity(arity) {}

    Node* getTarget() const { return target.get(); }

    const std::vector<NodePtr>& getElements() const { return elements; }
    CallArity getArity() const { return arity; }

   public:
    virtual ~UnaryChain() = default;
};

/**
 * Represents a chain of binary message sends in the AST, where each link in the
 * chain is a binary message sent to the target receiver of the previous
 * message.
 */
class BinaryChain : public Node {
   private:
    NodePtr target;
    std::vector<NodePtr> elements;
    CallArity arity;

   public:
    BinaryChain(NodePtr target, std::vector<NodePtr> elements, CallArity arity)
        : target(std::move(target)),
          elements(std::move(elements)),
          arity(arity) {}

    Node* getTarget() const { return target.get(); }

    const std::vector<NodePtr>& getElements() const { return elements; }
    CallArity getArity() const { return arity; }

   public:
    virtual ~BinaryChain() = default;
};

/**
 * Represents a Objective-C-style selector call in the AST.
 *
 * The target is the receiver of the selector call, the selector is the message
 * being invoked, and the arguments are the parameters passed to the selector.
 *
 * The arity indicates whether the call is unary, binary, or keyword-based.
 */
class SelectorCall : public Node {
   private:
    NodePtr target;
    std::string_view selector;
    std::vector<ArgumentPair<std::string_view, NodePtr>> arguments;
    CallArity arity;

   public:
    SelectorCall(NodePtr target, std::string_view selector,
                 std::vector<ArgumentPair<std::string_view, NodePtr>> arguments,
                 CallArity arity)
        : target(std::move(target)),
          selector(selector),
          arguments(std::move(arguments)),
          arity(arity) {}

    Node* getTarget() const { return target.get(); }
    std::string_view getSelector() const { return selector; }
    const std::vector<ArgumentPair<std::string_view, NodePtr>>& getArguments()
        const {
        return arguments;
    }
    CallArity getArity() const { return arity; }

   public:
    virtual ~SelectorCall() = default;
};
