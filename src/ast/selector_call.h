#pragma once

#include <string>
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
    NodePtr takeValue() { return std::move(value); }
};

class UnarySelector : public Node {
   private:
    std::string_view selector;

   public:
    UnarySelector(std::string_view selector) : selector(selector) {}

    std::string_view getSelector() const { return selector; }

   public:
    virtual ~UnarySelector() = default;
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
    std::string selector;
    std::vector<ArgumentPair<std::string_view, NodePtr>> arguments;
    CallArity arity;

   public:
    SelectorCall(NodePtr target,
                 std::string selector,
                 std::vector<ArgumentPair<std::string_view, NodePtr>> arguments,
                 CallArity arity)
        : target(std::move(target)),
          selector(std::move(selector)),
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
