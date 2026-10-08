#pragma once

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <string_view>

enum class TokenType {
    Eof,
    Unknown,
    Whitespace,
    Error,
    Comment,

    // Literals
    Ident,
    Number,
    Float,
    Char,
    String,

    // Operators
    Plus,
    Minus,
    Star,
    Slash,
    Equal,
    Bang,
    Less,
    Greater,
    LessEqual,
    GreaterEqual,
    EqualEqual,
    BangEqual,
    LParen,
    RParen,
    LBrace,
    RBrace,
    LBracket,
    RBracket,
    Dot,
    DotDot,
    Comma,
    Colon,
    Semicolon,

    // Keywords
    KwTrue,
    KwFalse,
    KwIf,
    KwElse,
    KwWhile,
    KwFor,
    KwReturn,
    KwLet,
    KwDef,
    KwEnum,
    KwClass,
    KwProtocol,
    KwConforms,
    KwField,
    KwMethod,
    KwPrivate,
    KwPublic,
    KwOverride,
};

/** Reserved keywords in the language */
static const char* reservedKeywords[] = {
    "true",     "false", "if",     "else",    "while",  "for",
    "return",   "let",   "def",    "enum",    "class",  "protocol",
    "conforms", "field", "method", "private", "public", "override",
};

class Token {
   public:
    TokenType type;
    std::string_view lexeme;
    size_t length;

    Token(TokenType type, const char* lexeme, size_t length)
        : type(type), lexeme(lexeme, length), length(length) {}

    Token(TokenType type, std::string_view lexeme)
        : type(type), lexeme(lexeme), length(lexeme.size()) {}

    Token(TokenType type) : type(type), lexeme(""), length(0) {}

    // Destructor
    ~Token() = default;

    /** Returns true if the token is a reserved keyword */
    bool isReservedKeyword() const {
        return std::any_of(
            std::begin(reservedKeywords), std::end(reservedKeywords),
            [this](const char* keyword) { return lexeme == keyword; });
    }

    /** Returns true if the token is an identifier */
    bool isIdentifier() const { return type == TokenType::Ident; }

    std::string_view tokenTypeToString() const {
        switch (type) {
            case TokenType::Eof:
                return "Eof";
            case TokenType::Unknown:
                return "Unknown";
            case TokenType::Whitespace:
                return "Whitespace";
            case TokenType::Comment:
                return "Comment";
            case TokenType::Ident:
                return "Ident";
            case TokenType::Number:
                return "Number";
            case TokenType::Float:
                return "Float";
            case TokenType::Char:
                return "Char";
            case TokenType::String:
                return "String";

            // Operators
            case TokenType::Plus:
                return "Plus";
            case TokenType::Minus:
                return "Minus";
            case TokenType::Star:
                return "Star";
            case TokenType::Slash:
                return "Slash";
            case TokenType::Equal:
                return "Equal";
            case TokenType::Bang:
                return "Bang";
            case TokenType::Less:
                return "Less";
            case TokenType::Greater:
                return "Greater";
            case TokenType::LessEqual:
                return "LessEqual";
            case TokenType::GreaterEqual:
                return "GreaterEqual";
            case TokenType::EqualEqual:
                return "EqualEqual";
            case TokenType::BangEqual:
                return "BangEqual";
            case TokenType::LParen:
                return "LParen";
            case TokenType::RParen:
                return "RParen";
            case TokenType::LBrace:
                return "LBrace";
            case TokenType::RBrace:
                return "RBrace";
            case TokenType::LBracket:
                return "LBracket";
            case TokenType::RBracket:
                return "RBracket";
            case TokenType::Dot:
                return "Dot";
            case TokenType::DotDot:
                return "DotDot";
            case TokenType::Comma:
                return "Comma";
            case TokenType::Colon:
                return "Colon";
            case TokenType::Semicolon:
                return "Semicolon";

            // Keywords
            case TokenType::KwTrue:
                return "KwTrue";
            case TokenType::KwFalse:
                return "KwFalse";
            case TokenType::KwIf:
                return "KwIf";
            case TokenType::KwElse:
                return "KwElse";
            case TokenType::KwWhile:
                return "KwWhile";
            case TokenType::KwFor:
                return "KwFor";
            case TokenType::KwReturn:
                return "KwReturn";
            case TokenType::KwLet:
                return "KwLet";
            case TokenType::KwDef:
                return "KwDef";
            case TokenType::KwEnum:
                return "KwEnum";
            case TokenType::KwClass:
                return "KwClass";
            case TokenType::KwProtocol:
                return "KwProtocol";
            case TokenType::KwConforms:
                return "KwConforms";
            case TokenType::KwField:
                return "KwField";
            case TokenType::KwMethod:
                return "KwMethod";
            case TokenType::KwPrivate:
                return "KwPrivate";
            case TokenType::KwPublic:
                return "KwPublic";
            case TokenType::KwOverride:
                return "KwOverride";
        }
        return "Unknown";
    }
};