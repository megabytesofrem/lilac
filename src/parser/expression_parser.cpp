#include "../parser.h"

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

std::expected<NodePtr, ParseError> Parser::parseTerm() {
    /* TODO: Implement expression parsing */
    return parseFactor();
}

std::expected<NodePtr, ParseError> Parser::parseFactor() {
    /* TODO: Implement factor parsing */
    return parsePrimary();
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
        case TokenType::Number:
        case TokenType::Float:
        case TokenType::String:
            return parseLiteral();
        case TokenType::Ident:
            return parseIdentifier();
        case TokenType::Symbol:
            return parseSelectorCall();
        default:
            return std::unexpected(ParseError::UnexpectedToken);
    }
}

std::expected<NodePtr, ParseError> Parser::parseExpression() {
    return parsePrimary();
}