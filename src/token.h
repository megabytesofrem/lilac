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
    Symbol,

    // Keywords
    KwTrue,
    KwFalse,
    KwIf,
    KwElse,
    KwWhile,
    KwFor,
    KwForeach,
    KwIn,
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

    // Type names
    KwI16,
    KwI32,
    KwI64,
    KwU16,
    KwU32,
    KwU64,
    KwF32,
    KwF64,
    KwBool,
    KwString,
    KwVoid
};

enum class Symbol {
    // Sentinel: Not a symbol
    None,

    Plus,
    Minus,
    Star,
    Slash,
    Equal,
    Bang,
    Tilde,
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
};

/** Reserved keywords in the language */
static const char* reservedKeywords[] = {
    "true",   "false",  "if",      "else",   "while",    "for",      "foreach",
    "in",     "return", "let",    "def",     "enum",   "class",    "protocol", "conforms",
    "field",  "method", "private", "public", "override",
};

class Token {
   public:
    TokenType type;
    Symbol symbol = Symbol::None;

    std::string_view lexeme;
    size_t length;

    Token(TokenType type, const char* lexeme, size_t length)
        : type(type), lexeme(lexeme, length), length(length) {}

    Token(TokenType type, Symbol symbol, size_t length)
        : type(type), symbol(symbol), lexeme(""), length(length) {}

    Token(TokenType type, std::string_view lexeme)
        : type(type), lexeme(lexeme), length(lexeme.size()) {}

    Token(TokenType type, std::string_view lexeme, size_t length)
        : type(type), lexeme(lexeme), length(length) {}

    Token(TokenType type) : type(type), lexeme(""), length(0) {}

    // Destructor
    ~Token() = default;

    /** Returns true if the token is a reserved keyword */
    bool isReservedKeyword() const {
        return std::any_of(
            std::begin(reservedKeywords), std::end(reservedKeywords),
            [this](const char* keyword) { return lexeme == keyword; });
    }

    /** Returns true if the token is a symbol */
    bool isSymbol() const { return type == TokenType::Symbol; }

    /** Returns true if the token is an identifier */
    bool isIdentifier() const { return type == TokenType::Ident; }

    std::string_view describeSymbol() const {
        switch (symbol) {
            case Symbol::None:
                return "None";
            case Symbol::Plus:
                return "Plus";
            case Symbol::Minus:
                return "Minus";
            case Symbol::Star:
                return "Star";
            case Symbol::Slash:
                return "Slash";
            case Symbol::Equal:
                return "Equal";
            case Symbol::Bang:
                return "Bang";
            case Symbol::Tilde:
                return "Tilde";
            case Symbol::Less:
                return "Less";
            case Symbol::Greater:
                return "Greater";
            case Symbol::LessEqual:
                return "LessEqual";
            case Symbol::GreaterEqual:
                return "GreaterEqual";
            case Symbol::EqualEqual:
                return "EqualEqual";
            case Symbol::BangEqual:
                return "BangEqual";
            case Symbol::LParen:
                return "LParen";
            case Symbol::RParen:
                return "RParen";
            case Symbol::LBrace:
                return "LBrace";
            case Symbol::RBrace:
                return "RBrace";
            case Symbol::LBracket:
                return "LBracket";
            case Symbol::RBracket:
                return "RBracket";
            case Symbol::Dot:
                return "Dot";
            case Symbol::DotDot:
                return "DotDot";
            case Symbol::Comma:
                return "Comma";
            case Symbol::Colon:
                return "Colon";
            case Symbol::Semicolon:
                return "Semicolon";
        }
        return "Unknown";
    }

    std::string_view describeTokenType() const {
        switch (type) {
            case TokenType::Eof:
                return "Eof";
            case TokenType::Unknown:
                return "Unknown";
            case TokenType::Whitespace:
                return "Whitespace";
            case TokenType::Error:
                return "Error";
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
            case TokenType::Symbol:
                return describeSymbol();

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
            case TokenType::KwForeach:
                return "KwForeach";
            case TokenType::KwIn:
                return "KwIn";
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
            case TokenType::KwI16:
                return "KwI16";
            case TokenType::KwI32:
                return "KwI32";
            case TokenType::KwI64:
                return "KwI64";
            case TokenType::KwU16:
                return "KwU16";
            case TokenType::KwU32:
                return "KwU32";
            case TokenType::KwU64:
                return "KwU64";
            case TokenType::KwF32:
                return "KwF32";
            case TokenType::KwF64:
                return "KwF64";
            case TokenType::KwBool:
                return "KwBool";
            case TokenType::KwString:
                return "KwString";
            case TokenType::KwVoid:
                return "KwVoid";
        }
        return "Unknown";
    }
};