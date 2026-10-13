#pragma once

#include <cstddef>
#include <deque>
#include <expected>
#include <memory>
#include <optional>
#include <vector>

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
    std::deque<Token> bufferedTokens;

    std::optional<Token> peekToken(size_t offset = 0);

    // Parser methods
    std::expected<NodePtr, ParseError> parseIntegerLiteral();
    std::expected<NodePtr, ParseError> parseFloatLiteral();
    std::expected<NodePtr, ParseError> parseStringLiteral();
    std::expected<NodePtr, ParseError> parseLiteral();
    std::expected<NodePtr, ParseError> parseIdentifier();

    std::expected<NodePtr, ParseError> parseExpression();
    std::expected<NodePtr, ParseError> parsePrimary();
    std::expected<KeywordArgument, ParseError> parseKeywordArgument();
    std::expected<std::vector<KeywordArgument>, ParseError>
    parseKeywordSelector();
    std::expected<std::unique_ptr<UnarySelectorExpr>, ParseError>
    parseUnarySelector();
    std::expected<NodePtr, ParseError> parseSelectorCall();

   public:
    Parser(Lexer lexer) : lexer(lexer) {}

    std::expected<NodePtr, ParseError> parse();
    std::expected<Token, ParseError> expect(TokenType type);
    std::expected<Token, ParseError> expectSymbol(Symbol sym);

    std::optional<Token> peek(size_t offset = 0);
    std::optional<Token> next();
};