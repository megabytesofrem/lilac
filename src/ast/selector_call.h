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
    std::string _keyword;
    NodePtr _value;

   public:
    KeywordArgument(std::string_view keyword, NodePtr value)
        : _keyword(keyword), _value(std::move(value)) {}

    std::string_view getKeyword() const { return _keyword; }
    ASTNode* getValue() const { return _value.get(); }
    NodePtr takeValue() { return std::move(_value); }
};

class UnarySelector : public ASTNode {
   private:
    std::string _selector;

   public:
    UnarySelector(std::string_view selector) : _selector(selector) {}

    std::string_view getSelector() const { return _selector; }

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
class SelectorCall : public ASTNode {
   private:
    NodePtr _target;
    std::string _selector;
    std::vector<ArgumentPair<std::string, NodePtr>> _arguments;
    CallArity _arity;

   public:
    SelectorCall(NodePtr target,
                 std::string selector,
                 std::vector<ArgumentPair<std::string, NodePtr>> arguments,
                 CallArity arity)
        : _target(std::move(target)),
          _selector(std::move(selector)),
          _arguments(std::move(arguments)),
          _arity(arity) {}

    ASTNode* getTarget() const { return _target.get(); }
    std::string_view getSelector() const { return _selector; }

    const std::vector<ArgumentPair<std::string, NodePtr>>& getArguments()
        const {
        return _arguments;
    }
    CallArity getArity() const { return _arity; }

   public:
    virtual ~SelectorCall() = default;
};
