#pragma once

#include "ast.h"

class AstVisitor {
   public:
    virtual ~AstVisitor() = default;

    void visit(Node* node) {
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
            for (Node* element : array->getElements()) {
                visit(element);
            }
        } else if (auto* call = dynamic_cast<CallExpr*>(node)) {
            visitCallExpr(*call);
            visit(call->getCallee());
            for (Node* argument : call->getArguments()) {
                visit(argument);
            }
        } else if (auto* let = dynamic_cast<LetExpr*>(node)) {
            visitLetExpr(*let);
            visit(let->getValue());
            visit(let->getBody());
        } else if (auto* assignment = dynamic_cast<AssignExpr*>(node)) {
            visitAssignExpr(*assignment);
            visit(assignment->getValue());
        } else if (auto* selector = dynamic_cast<SelectorCall*>(node)) {
            visitSelectorCall(*selector);
            visit(selector->getTarget());
            for (const auto& argument : selector->getArguments()) {
                visit(argument.getValue());
            }
        }
    }

    void visit(Item* item) {
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
            for (const MessageHandler& handler : implementation->getHandlers()) {
                visitMessageHandler(handler);
                visitMessage(handler.message);
                visitMessageArguments(handler.message);
                visit(handler.handler);
            }
        }
    }

   protected:
    virtual void visitNode(Node&) {}
    virtual void visitItem(Item&) {}

    virtual void visitLiteralExpr(LiteralExpr&) {}
    virtual void visitIdentifierExpr(IdentifierExpr&) {}
    virtual void visitArrayExpr(ArrayExpr&) {}
    virtual void visitCallExpr(CallExpr&) {}
    virtual void visitLetExpr(LetExpr&) {}
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
            visit(argument.getValue());
        }
    }
};
