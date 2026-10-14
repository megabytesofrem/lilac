#include "parser.h"

#include <memory>
#include <string>
#include <utility>

std::optional<Token> Parser::peekToken(size_t offset) {
    while (bufferedTokens.size() <= offset) {
        auto token = lexer.next();
        if (!token.has_value()) {
            return std::nullopt;
        }
        if (token->type == TokenType::Whitespace ||
            token->type == TokenType::Comment) {
            continue;
        }
        bufferedTokens.push_back(*token);
        if (token->type == TokenType::Eof && bufferedTokens.size() <= offset) {
            return std::nullopt;
        }
    }
    return bufferedTokens[offset];
}

/**
 * Peek at the next token from the input stream without advancing the parser.
 */
std::optional<Token> Parser::peek(size_t offset) { return peekToken(offset); }

/**
 * Return the next token from the input stream, skipping whitespace and comments.
 * 
 * This advances the parser to the next token.
 */
std::optional<Token> Parser::next() {
    auto token = peekToken();
    if (token.has_value()) {
        bufferedTokens.pop_front();
    }
    return token;
}

/**
 * Expect a token of the specified type from the input stream.
 */
std::expected<Token, ParseError> Parser::expect(TokenType type) {
    auto token = next();
    if (!token.has_value() || token->type == TokenType::Eof) {
        return std::unexpected(ParseError::EndOfInput);
    }
    if (token->type != type) {
        return std::unexpected(ParseError::UnexpectedToken);
    }
    return *token;
}

/**
 * Expect a symbol of the specified type from the input stream.
 */
std::expected<Token, ParseError> Parser::expectSymbol(Symbol sym) {
    auto token = next();
    if (!token.has_value() || token->type == TokenType::Eof) {
        return std::unexpected(ParseError::EndOfInput);
    }
    if (token->type != TokenType::Symbol || token->symbol != sym) {
        return std::unexpected(ParseError::UnexpectedToken);
    }
    return *token;
}

// MARK: Parser Methods

std::expected<NodePtr, ParseError> Parser::parse() {
    return parseStatement();
}