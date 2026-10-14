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
    Parser precedenceParser(Lexer("1 + 2 * 3"));
    auto precedenceResult = precedenceParser.parse();
    check(precedenceResult.has_value());
    auto* addition = dynamic_cast<BinExpr*>(precedenceResult->get());
    check(addition != nullptr);
    check(addition->getOperator() == "+");
    auto* additionLeft = dynamic_cast<LiteralExpr*>(addition->getLeft());
    check(additionLeft != nullptr);
    check(std::get<int>(additionLeft->getValue()) == 1);
    auto* multiplication = dynamic_cast<BinExpr*>(addition->getRight());
    check(multiplication != nullptr);
    check(multiplication->getOperator() == "*");
    auto* multiplicationLeft =
        dynamic_cast<LiteralExpr*>(multiplication->getLeft());
    auto* multiplicationRight =
        dynamic_cast<LiteralExpr*>(multiplication->getRight());
    check(multiplicationLeft != nullptr);
    check(multiplicationRight != nullptr);
    check(std::get<int>(multiplicationLeft->getValue()) == 2);
    check(std::get<int>(multiplicationRight->getValue()) == 3);

    Parser comparisonParser(Lexer("1 + 2 < 3 == 4"));
    auto comparisonResult = comparisonParser.parse();
    check(comparisonResult.has_value());
    auto* equality = dynamic_cast<BinExpr*>(comparisonResult->get());
    check(equality != nullptr);
    check(equality->getOperator() == "==");
    auto* lessThan = dynamic_cast<BinExpr*>(equality->getLeft());
    check(lessThan != nullptr);
    check(lessThan->getOperator() == "<");
    auto* additionInsideComparison =
        dynamic_cast<BinExpr*>(lessThan->getLeft());
    check(additionInsideComparison != nullptr);
    check(additionInsideComparison->getOperator() == "+");

    Parser associativityParser(Lexer("9 - 4 - 1"));
    auto associativityResult = associativityParser.parse();
    check(associativityResult.has_value());
    auto* outerSubtraction = dynamic_cast<BinExpr*>(associativityResult->get());
    check(outerSubtraction != nullptr);
    check(outerSubtraction->getOperator() == "-");
    auto* innerSubtraction = dynamic_cast<BinExpr*>(outerSubtraction->getLeft());
    check(innerSubtraction != nullptr);
    check(innerSubtraction->getOperator() == "-");

    Parser parenthesesParser(Lexer("(1 + 2) * 3"));
    auto parenthesesResult = parenthesesParser.parse();
    check(parenthesesResult.has_value());
    auto* multiplicationWithParentheses =
        dynamic_cast<BinExpr*>(parenthesesResult->get());
    check(multiplicationWithParentheses != nullptr);
    check(multiplicationWithParentheses->getOperator() == "*");
    auto* parenthesizedAddition = dynamic_cast<BinExpr*>(
        multiplicationWithParentheses->getLeft());
    check(parenthesizedAddition != nullptr);
    check(parenthesizedAddition->getOperator() == "+");

    Parser assignmentParser(Lexer("x = y = 1"));
    auto assignmentResult = assignmentParser.parse();
    check(assignmentResult.has_value());
    auto* outerAssignment =
        dynamic_cast<AssignExpr*>(assignmentResult->get());
    check(outerAssignment != nullptr);
    check(outerAssignment->getName() == "x");
    auto* innerAssignment =
        dynamic_cast<AssignExpr*>(outerAssignment->getValue());
    check(innerAssignment != nullptr);
    check(innerAssignment->getName() == "y");

    Parser prefixParser(Lexer("-1 * 2"));
    auto prefixResult = prefixParser.parse();
    check(prefixResult.has_value());
    auto* unaryMultiplication =
        dynamic_cast<BinExpr*>(prefixResult->get());
    check(unaryMultiplication != nullptr);
    check(unaryMultiplication->getOperator() == "*");
    auto* unaryMinus = dynamic_cast<UnaryExpr*>(unaryMultiplication->getLeft());
    check(unaryMinus != nullptr);
    check(unaryMinus->getOperator() == "-");
    auto* unaryOperand = dynamic_cast<LiteralExpr*>(unaryMinus->getOperand());
    check(unaryOperand != nullptr);
    check(std::get<int>(unaryOperand->getValue()) == 1);

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
