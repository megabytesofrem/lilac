#pragma once

#include "ast.h"

class AstVisitor {
   public:
    virtual ~AstVisitor() = default;

    void visit(ASTNode* node) {
        if (node == nullptr) {
            return;
        }

        visitNode(*node);

        if (auto* literal = dynamic_cast<LiteralExpr*>(node)) {
            visitLiteralExpr(*literal);
        } else if (auto* identifier = dynamic_cast<IdentifierExpr*>(node)) {
            visitIdentifierExpr(*identifier);
        } else if (auto* array = dynamic_cast<ArrayExpr*>(node)) {
            visitArrayExpr(*array);
            for (const NodePtr& element : array->getElements()) {
                visit(element.get());
            }
        } else if (auto* call = dynamic_cast<CallExpr*>(node)) {
            visitCallExpr(*call);
            visit(call->getCallee());
            for (const NodePtr& argument : call->getArguments()) {
                visit(argument.get());
            }
        } else if (auto* binary = dynamic_cast<BinExpr*>(node)) {
            visitBinExpr(*binary);
            visit(binary->getLeft());
            visit(binary->getRight());
        } else if (auto* unary = dynamic_cast<UnaryExpr*>(node)) {
            visitUnaryExpr(*unary);
            visit(unary->getOperand());
        } else if (auto* let = dynamic_cast<LetStmt*>(node)) {
            visitLetStmt(*let);
            visit(let->value());
        } else if (auto* block = dynamic_cast<BlockStmt*>(node)) {
            visitBlockStmt(*block);
            for (const NodePtr& statement : block->getStatements()) {
                visit(statement.get());
            }
        } else if (auto* loop = dynamic_cast<ForStmt*>(node)) {
            visitForStmt(*loop);
            visit(loop->getInitializer().get());
            visit(loop->getCondition().get());
            visit(loop->getIncrement().get());
            visit(loop->getBody().get());
        } else if (auto* loop = dynamic_cast<ForeachStmt*>(node)) {
            visitForeachStmt(*loop);
            visit(loop->getCollection().get());
            visit(loop->getBody().get());
        } else if (auto* loop = dynamic_cast<WhileStmt*>(node)) {
            visitWhileStmt(*loop);
            visit(loop->getCondition().get());
            visit(loop->getBody().get());
        } else if (auto* assignment = dynamic_cast<AssignExpr*>(node)) {
            visitAssignExpr(*assignment);
            visit(assignment->getValue());
        } else if (auto* selector = dynamic_cast<SelectorCall*>(node)) {
            visitSelectorCall(*selector);
            visit(selector->getTarget());
            for (const auto& argument : selector->getArguments()) {
                visit(argument.getValue().get());
            }
        }
    }

    void visit(ASTItem* item) {
        if (item == nullptr) {
            return;
        }

        visitItem(*item);

        if (auto* expression = dynamic_cast<ExprItem*>(item)) {
            visitExprItem(*expression);
            visit(expression->getExpression());
        } else if (auto* definition = dynamic_cast<ClassDefinition*>(item)) {
            visitClassDefinition(*definition);
            for (const ClassMethod& method : definition->getMethods()) {
                visitClassMethod(method);
                visit(method.getBody());
            }
        } else if (auto* definition = dynamic_cast<ProtocolDefinition*>(item)) {
            visitProtocolDefinition(*definition);
            for (const Message& message : definition->getMessages()) {
                visitMessage(message);
                visitMessageArguments(message);
            }
        } else if (auto* implementation =
                       dynamic_cast<ProtocolImplementation*>(item)) {
            visitProtocolImplementation(*implementation);
            for (const MessageHandler& handler :
                 implementation->getHandlers()) {
                visitMessageHandler(handler);
                visitMessage(handler.message);
                visitMessageArguments(handler.message);
                visit(handler.handler.get());
            }
        }
    }

   protected:
    virtual void visitNode(ASTNode&) {}
    virtual void visitItem(ASTItem&) {}

    virtual void visitLiteralExpr(LiteralExpr&) {}
    virtual void visitIdentifierExpr(IdentifierExpr&) {}
    virtual void visitArrayExpr(ArrayExpr&) {}
    virtual void visitCallExpr(CallExpr&) {}
    virtual void visitBinExpr(BinExpr&) {}
    virtual void visitUnaryExpr(UnaryExpr&) {}
    virtual void visitLetStmt(LetStmt&) {}
    virtual void visitBlockStmt(BlockStmt&) {}
    virtual void visitForStmt(ForStmt&) {}
    virtual void visitForeachStmt(ForeachStmt&) {}
    virtual void visitWhileStmt(WhileStmt&) {}
    virtual void visitAssignExpr(AssignExpr&) {}
    virtual void visitSelectorCall(SelectorCall&) {}

    virtual void visitExprItem(ExprItem&) {}
    virtual void visitClassDefinition(ClassDefinition&) {}
    virtual void visitClassMethod(const ClassMethod&) {}
    virtual void visitProtocolDefinition(ProtocolDefinition&) {}
    virtual void visitProtocolImplementation(ProtocolImplementation&) {}
    virtual void visitMessage(const Message&) {}
    virtual void visitMessageHandler(const MessageHandler&) {}

   private:
    void visitMessageArguments(const Message& message) {
        for (const auto& argument : message.keyword_args) {
            visit(argument.getValue().get());
        }
    }
};
