#include "../parser.h"

namespace {

struct InfixOperator {
    std::string_view op;
    int precedence;
    bool rightAssociative;
};

std::optional<InfixOperator> getInfixOperator(Symbol symbol) {
    switch (symbol) {
        case Symbol::Equal:
            return InfixOperator{"=", 1, true};
        case Symbol::EqualEqual:
            return InfixOperator{"==", 2, false};
        case Symbol::BangEqual:
            return InfixOperator{"!=", 2, false};
        case Symbol::Less:
            return InfixOperator{"<", 3, false};
        case Symbol::LessEqual:
            return InfixOperator{"<=", 3, false};
        case Symbol::Greater:
            return InfixOperator{">", 3, false};
        case Symbol::GreaterEqual:
            return InfixOperator{">=", 3, false};
        case Symbol::Plus:
            return InfixOperator{"+", 4, false};
        case Symbol::Minus:
            return InfixOperator{"-", 4, false};
        case Symbol::Star:
            return InfixOperator{"*", 5, false};
        case Symbol::Slash:
            return InfixOperator{"/", 5, false};
        default:
            return std::nullopt;
    }
}

std::optional<std::string_view> getPrefixOperator(Symbol symbol) {
    switch (symbol) {
        case Symbol::Plus:
            return "+";
        case Symbol::Minus:
            return "-";
        case Symbol::Bang:
            return "!";
        case Symbol::Tilde:
            return "~";
        default:
            return std::nullopt;
    }
}

}  // namespace

/**
 * Parse an integer literal i.e `123`
 */
std::expected<NodePtr, ParseError> Parser::parseIntegerLiteral() {
    auto token = expect(TokenType::Number);
    assert_expected(token);

    // Convert lexeme to integer value
    int value = std::stoi(token->lexeme.data());
    return std::make_unique<LiteralExpr>(value);
}

/**
 * Parse a float literal i.e `123.45`
 */
std::expected<NodePtr, ParseError> Parser::parseFloatLiteral() {
    auto token = expect(TokenType::Float);
    assert_expected(token);

    // Convert lexeme to float value
    float value = std::stof(token->lexeme.data());
    return std::make_unique<LiteralExpr>(value);
}

/**
 * Parse a string literal i.e `"hello world"`
 */
std::expected<NodePtr, ParseError> Parser::parseStringLiteral() {
    auto token = expect(TokenType::String);
    assert_expected(token);

    return std::make_unique<LiteralExpr>(token->lexeme);
}

/**
 * Parse an identifier i.e `variableName`
 */
std::expected<NodePtr, ParseError> Parser::parseIdentifier() {
    auto token = expect(TokenType::Ident);
    assert_expected(token);

    return std::make_unique<IdentifierExpr>(token->lexeme);
}

/**
 * Parse a literal, which can be an integer, float, or string.
 */
std::expected<NodePtr, ParseError> Parser::parseLiteral() {
    auto token = peek();
    if (!token.has_value()) {
        return std::unexpected(ParseError::EndOfInput);
    }

    switch (token->type) {
        case TokenType::Number:
            return parseIntegerLiteral();
        case TokenType::Float:
            return parseFloatLiteral();
        case TokenType::String:
            return parseStringLiteral();
        default:
            return std::unexpected(ParseError::UnexpectedToken);
    }
}

/**
 * Parse a primary expression, which can be a literal, identifier, or selector call.
 */
std::expected<NodePtr, ParseError> Parser::parsePrimary() {
    auto token = peek();
    if (!token.has_value() || token->type == TokenType::Eof) {
        return std::unexpected(ParseError::EndOfInput);
    }

    switch (token->type) {
        case TokenType::Symbol:
            /* Grouping */
            if (token->symbol == Symbol::LParen) {
                auto openParen = expectSymbol(Symbol::LParen);
                assert_expected(openParen);

                auto expression = parseExpression();
                assert_expected(expression);

                auto closeParen = expectSymbol(Symbol::RParen);
                assert_expected(closeParen);
                return expression;
            }

            /* Selector call */
            if (token->symbol == Symbol::LBracket) {
                return parseSelectorCall();
            }

            /* Unary operator */
            if (auto selector = getPrefixOperator(token->symbol)) {
                next();
                auto operand = parseExpression(6);
                assert_expected(operand);
                return std::make_unique<UnaryExpr>(std::string(*selector),
                                                   std::move(*operand));
            }
            return std::unexpected(ParseError::UnexpectedToken);
        case TokenType::Number:
        case TokenType::Float:
        case TokenType::String:
            return parseLiteral();
        case TokenType::Ident:
            return parseIdentifier();
        default:
            return std::unexpected(ParseError::UnexpectedToken);
    }
}

std::expected<NodePtr, ParseError> Parser::parseExpression(
    int minimumPrecedence) {
    auto left = parsePrimary();
    assert_expected(left);

    while (true) {
        auto token = peek();
        if (!token.has_value() || !token->isSymbol()) {
            break;
        }

        const auto infix = getInfixOperator(token->symbol);

        // Check if the predence is less than the minimum binding power.
        if (!infix.has_value() || infix->precedence < minimumPrecedence) {
            break;
        }
        next();

        const int nextMinimumPrecedence =
            infix->rightAssociative ? infix->precedence : infix->precedence + 1;
        auto right = parseExpression(nextMinimumPrecedence);
        assert_expected(right);

        // Combine the left and right expressions into a single expression node.
        NodePtr combined;

        if (token->symbol == Symbol::Equal) {
            /* Assignment */
            auto* identifier = dynamic_cast<IdentifierExpr*>(left->get());
            if (identifier == nullptr) {
                return std::unexpected(ParseError::UnexpectedToken);
            }
            combined = std::make_unique<AssignExpr>(identifier->getName(),
                                                    std::move(*right));
        } else {
            /* Binary operator */
            combined = std::make_unique<BinExpr>(
                std::string(infix->op), std::move(*left), std::move(*right));
        }
        left = std::move(combined);
    }

    return left;
}