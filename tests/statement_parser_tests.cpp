#include <cstdlib>
#include <iostream>
#include <string>
#include <string_view>
#include <variant>

#include "ast_visit.h"
#include "parser.h"

namespace {

void require(bool condition, std::string_view message) {
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

template <typename T>
T& NodeAs(ASTNode* node) {
    auto* result = dynamic_cast<T*>(node);
    require(result != nullptr, "Unexpected AST node type");
    return *result;
}

NodePtr parse(std::string_view source) {
    Parser parser{Lexer(source)};
    auto result = parser.parse();
    if (!result) {
        std::cerr << "Failed to parse: " << source << '\n';
        std::exit(EXIT_FAILURE);
    }
    auto next = parser.peek();
    require(next && next->type == TokenType::Eof, "Unconsumed input");
    return std::move(*result);
}

void testIdentifiers() {
    Parser parser{Lexer("  // comment\n _name123 = other")};
    auto name = parser.parseIdentifier();
    require(name && *name == "_name123", "Identifier spelling was lost");
    auto token = parser.peek();
    require(token && token->symbol == Symbol::Equal,
            "Identifier parser consumed the next token");

    for (std::string_view source :
         {"123", "let", "while", "for", "foreach", "in"}) {
        Parser invalid{Lexer(source)};
        auto result = invalid.parseIdentifier();
        require(!result && result.error() == ParseError::UnexpectedToken,
                "Non-identifier accepted as a name");
    }
    Parser empty{Lexer("")};
    auto result = empty.parseIdentifier();
    require(!result && result.error() == ParseError::EndOfInput,
            "Missing identifier did not report end of input");
}

void testLet() {
    // Test plain let statement without type annotation
    auto plain = parse("let count = source;");
    auto& binding = NodeAs<LetStmt>(plain.get());
    require(binding.name() == "count", "Incorrect binding name");
    require(!binding.type(), "Unannotated binding acquired a type");
    require(NodeAs<IdentifierExpr>(binding.value()).getName() == "source",
            "Initializer token was consumed by optional annotation parsing");

    // Test typed let statement with type annotation
    auto typed = parse("let count: I32 = 3");
    auto& annotated = NodeAs<LetStmt>(typed.get());
    require(annotated.type() && annotated.type()->type == Type::User &&
                annotated.type()->data.value == "I32",
            "Type annotation was not stored");
    require(
        std::get<int>(NodeAs<LiteralExpr>(annotated.value()).getValue()) == 3,
        "Incorrect annotated initializer");

    // Test nested pointer to an ARC type
    auto nested = parse("let pointer: *~I32 = value");
    const auto& type = *NodeAs<LetStmt>(nested.get()).type();
    require(type.type == Type::Pointer && type.data.childType &&
                type.data.childType->type == Type::ARC &&
                type.data.childType->data.childType &&
                type.data.childType->data.childType->data.value == "I32",
            "Nested pointer/ARC annotation was not parsed");
    TypeInfo copy = type;
    require(copy.data.childType.get() != type.data.childType.get() &&
                copy.data.childType->data.childType.get() !=
                    type.data.childType->data.childType.get(),
            "Copied type annotation shares ownership");
}

void testLoops() {
    auto adjacent = parse("{ let first = source; let second = first; second }");
    require(NodeAs<BlockStmt>(adjacent.get()).getStatements().size() == 3,
            "Block parser consumed or required a second statement terminator");
    auto adjacentLoops = parse("{ while (ready) {} for (;;) {} }");
    require(NodeAs<BlockStmt>(adjacentLoops.get()).getStatements().size() == 2,
            "Block parser required semicolons after loop bodies");

    // Test while loop parsing
    auto whileNode = parse("while (count < limit) { count = count + 1; }");
    auto& whileLoop = NodeAs<WhileStmt>(whileNode.get());
    require(
        NodeAs<BinExpr>(whileLoop.getCondition().get()).getOperator() == "<",
        "Incorrect while condition");
    auto& body = NodeAs<BlockStmt>(whileLoop.getBody().get());
    require(body.getStatements().size() == 1, "Incorrect while body");
    require(
        NodeAs<AssignExpr>(body.getStatements()[0].get()).getName() == "count",
        "Incorrect assignment name");

    // Test for loop parsing
    auto forNode =
        parse("for (let i: I32 = 0; i < limit; i = i + 1) { let item = i; }");
    auto& forLoop = NodeAs<ForStmt>(forNode.get());
    require(NodeAs<LetStmt>(forLoop.getInitializer().get()).name() == "i",
            "Incorrect for initializer");
    require(NodeAs<BinExpr>(forLoop.getCondition().get()).getOperator() == "<",
            "Incorrect for condition");
    require(NodeAs<AssignExpr>(forLoop.getIncrement().get()).getName() == "i",
            "Incorrect for increment");
    require(
        NodeAs<BlockStmt>(forLoop.getBody().get()).getStatements().size() == 1,
        "Incorrect for body");

    // Test for loop with optional components
    for (std::string_view source :
         {"for (;;) {}", "for (i = 0;;) {}", "for (;ready;) {}",
          "for (;;i = i + 1) {}"}) {
        auto node = parse(source);
        auto& loop = NodeAs<ForStmt>(node.get());
        require(NodeAs<BlockStmt>(loop.getBody().get()).getStatements().empty(),
                "Empty for body was not preserved");
        require(bool(loop.getInitializer()) == (source == "for (i = 0;;) {}"),
                "Incorrect optional initializer");
        require(bool(loop.getCondition()) == (source == "for (;ready;) {}"),
                "Incorrect optional condition");
        require(bool(loop.getIncrement()) == (source == "for (;;i = i + 1) {}"),
                "Incorrect optional increment");
    }

    // Test foreach loop parsing
    auto foreachNode =
        parse("foreach (item in collection) [Console show: item];");
    auto& foreachLoop = NodeAs<ForeachStmt>(foreachNode.get());
    require(foreachLoop.getVariable() == "item", "Incorrect foreach name");
    require(
        NodeAs<IdentifierExpr>(foreachLoop.getCollection().get()).getName() ==
            "collection",
        "Incorrect foreach collection");
    require(NodeAs<SelectorCall>(foreachLoop.getBody().get()).getSelector() ==
                "show:",
            "Foreach body must follow closing parenthesis");

    // Test nested statements parsing
    auto nested = parse(
        "{ let count = 0; while (true) for (;;) foreach (item in items) "
        "{ count = count + 1 } }");
    require(NodeAs<BlockStmt>(nested.get()).getStatements().size() == 2,
            "Nested statements were not parsed");
    auto single = parse("while (false) count = count + 1");
    require(!std::get<bool>(
                NodeAs<LiteralExpr>(
                    NodeAs<WhileStmt>(single.get()).getCondition().get())
                    .getValue()),
            "Boolean loop condition was not parsed");
}

void testErrors() {
    for (std::string_view source : {"let",
                                    "let 123 = 1",
                                    "let while = 1",
                                    "let x",
                                    "let x =",
                                    "let x: = 1",
                                    "let x: * = 1",
                                    "let x: ~ = 1",
                                    "let x: Number 1",
                                    "let x = ;",
                                    "let x = 1 let y = 2",
                                    "while",
                                    "while () {}",
                                    "while ready {}",
                                    "while (ready",
                                    "while (ready)",
                                    "while (ready) {",
                                    "while (ready) { x = ; }",
                                    "for",
                                    "for () {}",
                                    "for (let = 0;;) {}",
                                    "for (i = 0 i < 2;) {}",
                                    "for (i = 0; i < 2 i = i + 1) {}",
                                    "for (;;",
                                    "for (;;)",
                                    "for (;;i++) {}",
                                    "foreach",
                                    "foreach () {}",
                                    "foreach (123 in items) {}",
                                    "foreach (let in items) {}",
                                    "foreach (item items) {}",
                                    "foreach (item in) {}",
                                    "foreach (item in items {}",
                                    "foreach (item in items)",
                                    "{",
                                    "{ let x = 1;",
                                    "{ x y }",
                                    "[target 123]"}) {
        Parser parser{Lexer(source)};
        auto result = parser.parse();
        if (result) {
            std::cerr << "Malformed input was accepted: " << source << '\n';
            std::exit(EXIT_FAILURE);
        }
    }
}

void testExpressionBehavior() {
    auto node = parse("left = right = a + b * c");
    auto& left = NodeAs<AssignExpr>(node.get());
    auto& right = NodeAs<AssignExpr>(left.getValue());
    auto& addition = NodeAs<BinExpr>(right.getValue());
    require(left.getName() == "left" && right.getName() == "right" &&
                addition.getOperator() == "+" &&
                NodeAs<BinExpr>(addition.getRight()).getOperator() == "*",
            "Assignment associativity or arithmetic precedence changed");
    auto unary = parse("[target size]");
    require(NodeAs<SelectorCall>(unary.get()).getArity() == CallArity::Unary,
            "Unary selector behavior changed");
    auto keyword = parse("[target first: value second: other]");
    auto& call = NodeAs<SelectorCall>(keyword.get());
    require(call.getSelector() == "first:second:" &&
                call.getArguments().size() == 2 &&
                call.getArguments()[1].getKey() == "second",
            "Keyword selector behavior changed");
}

void testOwnedNames() {
    NodePtr node;
    {
        std::string source =
            "{ let binding: *Named = receiver; binding = receiver; "
            "foreach (element in collection) [receiver keyword: element]; }";
        node = parse(source);
        source.assign(source.size(), 'x');
    }
    const auto& statements = NodeAs<BlockStmt>(node.get()).getStatements();
    auto& binding = NodeAs<LetStmt>(statements[0].get());
    require(binding.name() == "binding" &&
                binding.type()->data.childType->data.value == "Named" &&
                NodeAs<IdentifierExpr>(binding.value()).getName() == "receiver",
            "Binding, type or identifier name borrows input storage");
    require(NodeAs<AssignExpr>(statements[1].get()).getName() == "binding",
            "Assignment name borrows input storage");
    auto& loop = NodeAs<ForeachStmt>(statements[2].get());
    auto& call = NodeAs<SelectorCall>(loop.getBody().get());
    require(loop.getVariable() == "element" &&
                NodeAs<IdentifierExpr>(loop.getCollection().get()).getName() ==
                    "collection" &&
                call.getSelector() == "keyword:" &&
                call.getArguments()[0].getKey() == "keyword",
            "Loop or selector name borrows input storage");

    std::string source = "standalone";
    Parser parser{Lexer(source)};
    auto name = parser.parseIdentifier();
    source.assign(source.size(), 'x');
    require(name && *name == "standalone", "Parsed name borrows input storage");
}

void testVisitor() {
    class CountingVisitor : public AstVisitor {
       public:
        int nodes = 0;

       protected:
        void visitNode(ASTNode&) override { ++nodes; }
    };

    auto node = parse(
        "{ let x = value; while (ready) { x = next; } "
        "for (;;) {} foreach (item in items) {} }");
    CountingVisitor visitor;
    visitor.visit(node.get());
    require(visitor.nodes == 13, "Visitor skipped statement children");
}

}  // namespace

int main() {
    testIdentifiers();
    testLet();
    testLoops();
    testErrors();
    testExpressionBehavior();
    testOwnedNames();
    testVisitor();
    std::cout << "Statement parser tests passed\n";
}
