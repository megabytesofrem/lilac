#pragma once

#include "ast_node.h"

/**
 * The arity of a selector call.
 *
 * Unary represents a call with one argument, binary represents a call with two
 * arguments and keyword is N-ary.
 */
enum class CallArity { Unary, Binary, Keyword };

/**
 * Represents a Smalltalk-style selector call in the AST.
 *
 * The target is the receiver of the selector call, the selector is the message
 * being invoked, and the arguments are the parameters passed to the selector.
 *
 * The arity indicates whether the call is unary, binary, or keyword-based.
 */
class SelectorCall : public Node {
   private:
    Node* target;
    std::string_view selector;
    std::vector<ArgumentPair<std::string_view, Node*>> arguments;
    CallArity arity;

   public:
    SelectorCall(
        Node* target, std::string_view selector,
        const std::vector<ArgumentPair<std::string_view, Node*>>& arguments,
        CallArity arity)
        : target(target),
          selector(selector),
          arguments(arguments),
          arity(arity) {}

    Node* getTarget() const { return target; }
    std::string_view getSelector() const { return selector; }
    const std::vector<ArgumentPair<std::string_view, Node*>>& getArguments()
        const {
        return arguments;
    }
    CallArity getArity() const { return arity; }

   public:
    virtual ~SelectorCall() = default;
};
