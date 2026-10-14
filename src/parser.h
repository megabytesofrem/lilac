#pragma once

#include <cstddef>
#include <deque>
#include <expected>
#include <memory>
#include <optional>
#include <vector>

#include "ast.h"
#include "lexer.h"

#define assert_expected(token) \
    if (!(token)) { \
        return std::unexpected((token).error()); \
    }

#define assert_not_expected(token) \
    if ((token)) { \
        return std::unexpected((token).error()); \
    }

enum class ParseError {
    UnexpectedToken,
    ExpectedAType,
    EndOfInput,
};

class Parser {
   private:
    Lexer lexer;
    std::deque<Token> bufferedTokens;

    std::optional<Token> peekToken(size_t offset = 0);

    // Parser methods
    std::expected<TypeInfo, ParseError> parseType();

    std::expected<NodePtr, ParseError> parseIntegerLiteral();
    std::expected<NodePtr, ParseError> parseFloatLiteral();
    std::expected<NodePtr, ParseError> parseStringLiteral();
    std::expected<NodePtr, ParseError> parseLiteral();
    std::expected<NodePtr, ParseError> parseIdentifier();
    std::expected<NodePtr, ParseError> parseFactor();
    std::expected<NodePtr, ParseError> parseTerm();
    std::expected<NodePtr, ParseError> parsePrimary();
    std::expected<NodePtr, ParseError> parseExpression();

    std::expected<KeywordArgument, ParseError> parseKeywordArgument();
    std::expected<std::vector<KeywordArgument>, ParseError> parseKeywordSel();
    std::expected<std::unique_ptr<UnarySelector>, ParseError> parseUnarySel();
    std::expected<NodePtr, ParseError> parseSelectorCall();

    std::expected<NodePtr, ParseError> parseLetStmt();

   public:
    Parser(Lexer lexer) : lexer(lexer) {}

    std::expected<NodePtr, ParseError> parse();
    std::expected<Token, ParseError> expect(TokenType type);
    std::expected<Token, ParseError> expectSymbol(Symbol sym);

    std::optional<Token> peek(size_t offset = 0);
    std::optional<Token> next();
};