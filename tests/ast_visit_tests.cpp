#include <cstdlib>
#include <memory>
#include <string_view>
#include <utility>
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
    auto literal = std::make_unique<LiteralExpr>(42);
    LiteralExpr floatingLiteral(3.5);
    LiteralExpr stringLiteral(std::string_view("text"));
    LiteralExpr charLiteral('x');
    LiteralExpr boolLiteral(true);

    check(std::get<int>(literal->getValue()) == 42);
    check(std::get<double>(floatingLiteral.getValue()) == 3.5);
    check(std::get<std::string_view>(stringLiteral.getValue()) == "text");
    check(std::get<char>(charLiteral.getValue()) == 'x');
    check(std::get<bool>(boolLiteral.getValue()) == true);

    auto identifier = std::make_unique<IdentifierExpr>("name");
    std::vector<NodePtr> callArguments;
    auto call = std::make_unique<CallExpr>(std::move(identifier),
                                           std::move(callArguments));
    std::vector<NodePtr> arrayElements;
    arrayElements.push_back(std::move(literal));
    arrayElements.push_back(std::move(call));
    auto array = std::make_unique<ArrayExpr>(std::move(arrayElements));
    auto expression = std::make_unique<LetExpr>(
        "value", std::make_unique<LiteralExpr>(42), std::move(array));

    CountingVisitor visitor;
    visitor.visit(expression.get());
    check(visitor.nodes == 6);
    check(visitor.literals == 2);
    check(visitor.identifiers == 1);

    std::vector<ArgumentPair<std::string_view, NodePtr>> messageArguments;
    messageArguments.emplace_back("argument",
                                  std::make_unique<IdentifierExpr>("name"));
    Message message("target", "selector", std::move(messageArguments));
    std::vector<Message> messages;
    messages.push_back(std::move(message));
    ProtocolDefinition definition("protocol", std::move(messages));
    ExprItem expressionItem(std::make_unique<LiteralExpr>(42));
    std::vector<ArgumentPair<std::string_view, NodePtr>> selectorArguments;
    selectorArguments.emplace_back("value", std::make_unique<LiteralExpr>(42));
    SelectorCall selector(std::make_unique<IdentifierExpr>("name"), "plus",
                          std::move(selectorArguments), CallArity::Keyword);
    AssignExpr assignment("result", std::make_unique<LiteralExpr>(42));
    std::vector<ClassMethod> methods;
    methods.emplace_back("method", ClassMethodVisibility::Public, Type::Void,
                         std::make_unique<AssignExpr>(
                             "result", std::make_unique<LiteralExpr>(42)));
    ClassDefinition classDefinition("Example", std::move(methods));
    std::vector<ArgumentPair<std::string_view, NodePtr>> handlerArguments;
    handlerArguments.emplace_back("argument",
                                  std::make_unique<IdentifierExpr>("name"));
    MessageHandler handler(
        Message("target", "selector", std::move(handlerArguments)),
        std::make_unique<IdentifierExpr>("name"));
    std::vector<MessageHandler> handlers;
    handlers.push_back(std::move(handler));
    ProtocolImplementation implementation("protocol", "Example",
                                          std::move(handlers));

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
