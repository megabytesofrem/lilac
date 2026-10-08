#pragma once

#include <expected>
#include <optional>
#include <string_view>

#include "token.h"

class Lexer {
   private:
    std::string_view source;
    size_t position = 0;

    bool isIdentifierStart(char ch) const;
    bool isIdentifierPart(char ch) const;
    Token extractKeyword(const std::string_view keyword) const;

    Token scanIdentifier() const;
    Token scanNumber() const;
    Token scanString() const;
    Token scanSymbol(char ch) const;

   public:
    // The source storage must outlive the lexer and its tokens.
    Lexer(const std::string_view source) : source(source), position(0) {}
    ~Lexer() = default;

    bool atEnd() const { return position >= source.size(); }

    // nullopt reports an unterminated or invalid literal.
    std::optional<Token> peek() const;
    std::optional<Token> next();
};