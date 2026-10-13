#include <cstdlib>
#include <string_view>
#include <variant>

#include "parser.h"

static void check(bool condition) {
    if (!condition) {
        std::abort();
    }
}

int main() {
    Parser keywordParser(Lexer(
        R"([NSString stringByAppending: "world" to: "hello"])"));
    auto keywordResult = keywordParser.parse();
    check(keywordResult.has_value());

    auto* keywordCall =
        dynamic_cast<SelectorCall*>(keywordResult->get());
    check(keywordCall != nullptr);
    check(keywordCall->getArity() == CallArity::Keyword);
    auto* keywordTarget =
        dynamic_cast<IdentifierExpr*>(keywordCall->getTarget());
    check(keywordTarget != nullptr);
    check(keywordTarget->getName() == "NSString");
    check(keywordCall->getSelector() == "stringByAppending:to:");
    check(keywordCall->getArguments().size() == 2);
    check(keywordCall->getArguments()[0].getKey() == "stringByAppending");
    check(keywordCall->getArguments()[1].getKey() == "to");

    auto* firstValue = dynamic_cast<LiteralExpr*>(
        keywordCall->getArguments()[0].getValue().get());
    auto* secondValue = dynamic_cast<LiteralExpr*>(
        keywordCall->getArguments()[1].getValue().get());
    check(firstValue != nullptr);
    check(secondValue != nullptr);
    check(std::get<std::string_view>(firstValue->getValue()) == "world");
    check(std::get<std::string_view>(secondValue->getValue()) == "hello");

    Parser unaryParser(Lexer("[Thing init]"));
    auto unaryResult = unaryParser.parse();
    check(unaryResult.has_value());

    auto* unaryCall = dynamic_cast<SelectorCall*>(unaryResult->get());
    check(unaryCall != nullptr);
    check(unaryCall->getArity() == CallArity::Unary);
    auto* unaryTarget = dynamic_cast<IdentifierExpr*>(unaryCall->getTarget());
    check(unaryTarget != nullptr);
    check(unaryTarget->getName() == "Thing");
    check(unaryCall->getSelector() == "init");
    check(unaryCall->getArguments().empty());

    return EXIT_SUCCESS;
}
