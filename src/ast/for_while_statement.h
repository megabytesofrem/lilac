#pragma once

#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

#include "ast_node.h"

/** 
 * A C style for loop statement in the AST.
 * 
 * ```
 * for (let i: type = 0; i < 10; i = i + 1) {
 *     // loop body
 * }
 * ```
 */
class ForStmt : public ASTNode {
   private:
    std::unique_ptr<ASTNode> _initializer;
    std::unique_ptr<ASTNode> _condition;
    std::unique_ptr<ASTNode> _increment;
    std::unique_ptr<ASTNode> _body;

   public:
    ForStmt(std::unique_ptr<ASTNode> initializer,
            std::unique_ptr<ASTNode> condition,
            std::unique_ptr<ASTNode> increment,
            std::unique_ptr<ASTNode> body)
        : _initializer(std::move(initializer)),
          _condition(std::move(condition)),
          _increment(std::move(increment)),
          _body(std::move(body)) {}

    const std::unique_ptr<ASTNode>& getInitializer() const {
        return _initializer;
    }
    const std::unique_ptr<ASTNode>& getCondition() const { return _condition; }
    const std::unique_ptr<ASTNode>& getIncrement() const { return _increment; }
    const std::unique_ptr<ASTNode>& getBody() const { return _body; }
};

/**
 * A modern foreach loop statement in the AST.
 *
 * ```
 * foreach (item in collection) {
 *     // loop body
 * }
 * ```
 */
class ForeachStmt : public ASTNode {
   private:
    std::string _variable;
    std::unique_ptr<ASTNode> _collection;
    std::unique_ptr<ASTNode> _body;

   public:
    ForeachStmt(std::string variable,
                std::unique_ptr<ASTNode> collection,
                std::unique_ptr<ASTNode> body)
        : _variable(std::move(variable)),
          _collection(std::move(collection)),
          _body(std::move(body)) {}

    std::string_view getVariable() const { return _variable; }
    const std::unique_ptr<ASTNode>& getCollection() const {
        return _collection;
    }
    const std::unique_ptr<ASTNode>& getBody() const { return _body; }
};

/**
 * A while loop statement in the AST.
 *
 * ```
 * while (condition) {
 *     // loop body
 * }
 * ```
 */
class WhileStmt : public ASTNode {
   private:
    std::unique_ptr<ASTNode> _condition;
    std::unique_ptr<ASTNode> _body;

   public:
    WhileStmt(std::unique_ptr<ASTNode> condition, std::unique_ptr<ASTNode> body)
        : _condition(std::move(condition)), _body(std::move(body)) {}

    const std::unique_ptr<ASTNode>& getCondition() const { return _condition; }
    const std::unique_ptr<ASTNode>& getBody() const { return _body; }
};

class BlockStmt : public ASTNode {
   private:
    std::vector<NodePtr> _statements;

   public:
    explicit BlockStmt(std::vector<NodePtr> statements)
        : _statements(std::move(statements)) {}

    const std::vector<NodePtr>& getStatements() const { return _statements; }
};