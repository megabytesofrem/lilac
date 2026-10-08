#include "lexer.h"

#include <cctype>
#include <iostream>

bool Lexer::isIdentifierStart(char ch) const {
    return std::isalpha(static_cast<unsigned char>(ch)) || ch == '_';
}

bool Lexer::isIdentifierPart(char ch) const {
    return std::isalnum(static_cast<unsigned char>(ch)) || ch == '_';
}

Token Lexer::extractKeyword(const std::string_view keyword) const {
    if (keyword == "true") {
        return Token(TokenType::KwTrue);
    } else if (keyword == "false") {
        return Token(TokenType::KwFalse);
    } else if (keyword == "if") {
        return Token(TokenType::KwIf);
    } else if (keyword == "else") {
        return Token(TokenType::KwElse);
    } else if (keyword == "while") {
        return Token(TokenType::KwWhile);
    } else if (keyword == "for") {
        return Token(TokenType::KwFor);
    } else if (keyword == "return") {
        return Token(TokenType::KwReturn);
    } else if (keyword == "let") {
        return Token(TokenType::KwLet);
    } else if (keyword == "def") {
        return Token(TokenType::KwDef);
    } else if (keyword == "enum") {
        return Token(TokenType::KwEnum);
    } else if (keyword == "class") {
        return Token(TokenType::KwClass);
    } else if (keyword == "protocol") {
        return Token(TokenType::KwProtocol);
    } else if (keyword == "conforms") {
        return Token(TokenType::KwConforms);
    } else if (keyword == "field") {
        return Token(TokenType::KwField);
    } else if (keyword == "method") {
        return Token(TokenType::KwMethod);
    } else if (keyword == "private") {
        return Token(TokenType::KwPrivate);
    } else if (keyword == "public") {
        return Token(TokenType::KwPublic);
    } else if (keyword == "override") {
        return Token(TokenType::KwOverride);
    }

    // Not a reserved keyword
    return Token(TokenType::Ident);
}

Token Lexer::scanIdentifier() const {
    Token id(TokenType::Ident);
    id.length = 1;

    // Continue scanning the identifier
    while (position + id.length < source.size() &&
           isIdentifierPart(source[position + id.length])) {
        id.length++;
    }
    id.lexeme = source.substr(position, id.length);
    // Check if the identifier is a reserved keyword
    if (id.isReservedKeyword()) {
        id.type = extractKeyword(id.lexeme).type;
    }

    return id;
}

Token Lexer::scanNumber() const {
    Token number(TokenType::Number);
    number.length = 1;

    while (position + number.length < source.size() &&
           std::isdigit(
               static_cast<unsigned char>(source[position + number.length]))) {
        number.length++;
    }

    // Floating-point numbers
    if (position + number.length < source.size() &&
        source[position + number.length] == '.') {
        number.type = TokenType::Float;
        number.length++;
        while (position + number.length < source.size() &&
               std::isdigit(static_cast<unsigned char>(
                   source[position + number.length]))) {
            number.length++;
        }
    }

    return Token(number.type, source.substr(position, number.length));
}

Token Lexer::scanString() const {
    Token str(TokenType::String);
    str.length = 1;  // Eat the opening quote
    while (position + str.length < source.size() &&
           source[position + str.length] != '\0' &&
           source[position + str.length] != '"') {
        str.length++;
    }
    if (position + str.length >= source.size() ||
        source[position + str.length] != '"') {
        return Token(TokenType::Unknown);
    }

    size_t expected_length = str.length + 1;  // Including the closing quote

    str.length++;  // Eat the closing quote

    if (str.length != expected_length) {
        std::cout << "Unterminated string literal\n";
        return Token(TokenType::Error);
    }

    // Strip the quotes from the string literal
    str.lexeme = source.substr(position + 1, str.length - 2);

    return Token(str.type, source.substr(position, str.length));
}

Token Lexer::scanSymbol(char ch) const {
    TokenType type = TokenType::Unknown;
    size_t length = 1;
    const char next =
        position + 1 < source.size() ? source[position + 1] : '\0';
    switch (ch) {
        case '+':
            type = TokenType::Plus;
            break;
        case '-':
            type = TokenType::Minus;
            break;
        case '*':
            type = TokenType::Star;
            break;
        case '/':
            type = TokenType::Slash;
            break;
        case '=':
            type = next == '=' ? TokenType::EqualEqual : TokenType::Equal;
            length = next == '=' ? 2 : 1;
            break;
        case '!':
            type = next == '=' ? TokenType::BangEqual : TokenType::Bang;
            length = next == '=' ? 2 : 1;
            break;
        case '<':
            type = next == '=' ? TokenType::LessEqual : TokenType::Less;
            length = next == '=' ? 2 : 1;
            break;
        case '>':
            type = next == '=' ? TokenType::GreaterEqual : TokenType::Greater;
            length = next == '=' ? 2 : 1;
            break;
        case '(':
            type = TokenType::LParen;
            break;
        case ')':
            type = TokenType::RParen;
            break;
        case '{':
            type = TokenType::LBrace;
            break;
        case '}':
            type = TokenType::RBrace;
            break;
        case '[':
            type = TokenType::LBracket;
            break;
        case ']':
            type = TokenType::RBracket;
            break;
        case '.':
            type = next == '.' ? TokenType::DotDot : TokenType::Dot;
            length = next == '.' ? 2 : 1;
            break;
        case ',':
            type = TokenType::Comma;
            break;
        case ':':
            type = TokenType::Colon;
            break;
        case ';':
            type = TokenType::Semicolon;
            break;
        default:
            break;
    }
    return Token(type, source.substr(position, length));
}

std::optional<Token> Lexer::peek() const {
    if (this->atEnd()) {
        return Token(TokenType::Eof);
    }

    char ch = source[position];
    if (ch == '\0') {
        return Token(TokenType::Eof);
    }

    if (ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r') {
        return Token(TokenType::Whitespace, source.substr(position, 1));
    }

    // Line comments
    if (ch == '/' && position + 1 < source.size() &&
        source[position + 1] == '/') {
        Token comment(TokenType::Comment);
        comment.length = 2;  // Length of the "//" comment start
        while (position + comment.length < source.size() &&
               source[position + comment.length] != '\0' &&
               source[position + comment.length] != '\n') {
            comment.length++;
        }

        return Token(comment.type, source.substr(position, comment.length));
    }

    // Identifiers and keywords
    if (isIdentifierStart(ch)) {
        return scanIdentifier();
    }

    // Numbers: both integers and floats
    if (std::isdigit(static_cast<unsigned char>(ch))) {
        return scanNumber();
    }

    // String literals: "hello world"
    if (ch == '"') {
        return scanString();
    }

    // Characters: 'C'
    if (ch == '\'') {
        if (source.size() - position < 3 || source[position + 1] == '\0' ||
            source[position + 1] == '\'' || source[position + 2] != '\'') {
            return std::nullopt;
        }

        return Token(TokenType::Char, source.substr(position, 3));
    }

    // Symbols and operators
    return scanSymbol(ch);
}

std::optional<Token> Lexer::next() {
    if (this->atEnd()) {
        return Token(TokenType::Eof);
    }

    std::optional<Token> token = this->peek();
    if (token.has_value()) {
        this->position += token->length;
    }
    return token;
}