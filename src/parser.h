#pragma once

#include <expected>
#include <memory>
#include <optional>

#include "ast.h"
#include "lexer.h"

#define assert_expected(token)                   \
    if (!(token)) {                              \
        return std::unexpected((token).error()); \
    }

#define assert_not_expected(token)               \
    if ((token)) {                               \
        return std::unexpected((token).error()); \
    }

enum class ParseError {
    UnexpectedToken,
    EndOfInput,
};

class Parser {
   private:
    Lexer lexer;

    // Parser methods
    std::expected<NodePtr, ParseError> parseIntegerLiteral();
    std::expected<NodePtr, ParseError> parseFloatLiteral();
    std::expected<NodePtr, ParseError> parseStringLiteral();
    std::expected<NodePtr, ParseError> parseLiteral();
    std::expected<NodePtr, ParseError> parseIdentifier();

    std::expected<NodePtr, ParseError> parseExpression();
    std::expected<NodePtr, ParseError> parsePrimary();
    std::expected<KeywordArgument, ParseError> parseKeywordArgument();
    std::expected<NodePtr, ParseError> parseUnarySelector();
    std::expected<NodePtr, ParseError> parseUnaryChain();
    std::expected<NodePtr, ParseError> parseBinarySelector();
    std::expected<NodePtr, ParseError> parseBinaryChain();
    std::expected<NodePtr, ParseError> parseSelectorCall();

   public:
    Parser(Lexer lexer) : lexer(lexer) {}

    std::expected<Token, ParseError> look(TokenType type);
    std::expected<Token, ParseError> expect(TokenType type);

    std::optional<Token> peek() const;
    std::optional<Token> next();
};