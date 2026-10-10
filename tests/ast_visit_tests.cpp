#include <cstdlib>
#include <variant>

#include "ast_visit.h"

static void check(bool condition) {
    if (!condition) {
        std::abort();
    }
}

class CountingVisitor : public AstVisitor {
   public:
    int nodes = 0;
    int items = 0;
    int literals = 0;
    int identifiers = 0;
    int selectorCalls = 0;
    int classMethods = 0;
    int messages = 0;
    int messageHandlers = 0;

   protected:
    void visitNode(Node&) override { ++nodes; }
    void visitItem(Item&) override { ++items; }
    void visitLiteralExpr(LiteralExpr&) override { ++literals; }
    void visitIdentifierExpr(IdentifierExpr&) override { ++identifiers; }
    void visitSelectorCall(SelectorCall&) override { ++selectorCalls; }
    void visitClassMethod(const ClassMethod&) override { ++classMethods; }
    void visitMessage(const Message&) override { ++messages; }
    void visitMessageHandler(const MessageHandler&) override {
        ++messageHandlers;
    }
};

int main() {
    LiteralExpr literal(42);
    LiteralExpr floatingLiteral(3.5);
    LiteralExpr stringLiteral(std::string_view("text"));
    LiteralExpr charLiteral('x');
    LiteralExpr boolLiteral(true);

    check(std::get<int>(literal.getValue()) == 42);
    check(std::get<double>(floatingLiteral.getValue()) == 3.5);
    check(std::get<std::string_view>(stringLiteral.getValue()) == "text");
    check(std::get<char>(charLiteral.getValue()) == 'x');
    check(std::get<bool>(boolLiteral.getValue()) == true);

    IdentifierExpr identifier("name");
    CallExpr call(&identifier, {});
    ArrayExpr array({&literal, &call});
    LetExpr expression("value", &literal, &array);

    CountingVisitor visitor;
    visitor.visit(&expression);
    check(visitor.nodes == 6);
    check(visitor.literals == 2);
    check(visitor.identifiers == 1);

    Message message("target", "selector", {{"argument", &identifier}});
    ProtocolDefinition definition("protocol", {message});
    ExprItem expressionItem(&literal);
    SelectorCall selector(&identifier, "plus", {{"value", &literal}},
                          CallArity::Keyword);
    AssignExpr assignment("result", &literal);
    ClassDefinition classDefinition(
        "Example", {ClassMethod("method", ClassMethodVisibility::Public,
                                Type::Void, &assignment)});
    MessageHandler handler(message, &identifier);
    ProtocolImplementation implementation("protocol", "Example", {handler});

    visitor.visit(&definition);
    visitor.visit(&expressionItem);
    visitor.visit(&selector);
    visitor.visit(&assignment);
    visitor.visit(&classDefinition);
    visitor.visit(&implementation);
    check(visitor.items == 4);
    check(visitor.nodes == 17);
    check(visitor.literals == 6);
    check(visitor.identifiers == 5);
    check(visitor.selectorCalls == 1);
    check(visitor.classMethods == 1);
    check(visitor.messages == 2);
    check(visitor.messageHandlers == 1);

    visitor.visit(static_cast<Node*>(nullptr));
    visitor.visit(static_cast<Item*>(nullptr));
    check(visitor.nodes == 17);
    check(visitor.items == 4);
    return EXIT_SUCCESS;
}
