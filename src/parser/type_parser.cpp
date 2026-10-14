#include "../parser.h"

/**
 * Parse a type from the input stream into its TypeInfo representation.
 */
std::expected<TypeInfo, ParseError> Parser::parseType() {
    auto token = next();
    if (!token.has_value()) {
        return std::unexpected(ParseError::EndOfInput);
    }

    // Error: We expected a type, but got something else
    if (token->type == TokenType::Symbol && (token->symbol != Symbol::Star) &&
        (token->symbol != Symbol::Tilde)) {
        return std::unexpected(ParseError::ExpectedAType);
    }

    // Switch on the token type to build the corresponding TypeInfo.
    // TypeInfo carries the type information, including any nested types for pointers or ARC types.

    switch (token->type) {
        case TokenType::Symbol:
            /* *Type indicates a raw pointer */
            if (token->symbol == Symbol::Star) {
                auto innerType = this->parseType();
                if (!innerType.has_value()) {
                    return std::unexpected(ParseError::ExpectedAType);
                }

                // Make the inner type info a child of the pointer type
                return TypeInfo(Type::Pointer,
                                std::make_unique<TypeInfo>(innerType.value()));
            } else {
                return std::unexpected(ParseError::UnexpectedToken);
            }

            /* ~Type indicates an ARC type */
            if (token->symbol == Symbol::Tilde) {
                auto innerType = this->parseType();
                if (!innerType.has_value()) {
                    return std::unexpected(ParseError::ExpectedAType);
                }

                // Make the inner type info a child of the ARC type
                return TypeInfo(Type::ARC,
                                std::make_unique<TypeInfo>(innerType.value()));
            } else {
                return std::unexpected(ParseError::UnexpectedToken);
            }

        /* All other types are much simpler to handle */
        case TokenType::Ident:
            return TypeInfo(Type::User, token->lexeme);
        case TokenType::KwI16:
            return TypeInfo(Type::I16, token->lexeme);
        case TokenType::KwI32:
            return TypeInfo(Type::I32, token->lexeme);
        case TokenType::KwI64:
            return TypeInfo(Type::I64, token->lexeme);
        case TokenType::KwU16:
            return TypeInfo(Type::U16, token->lexeme);
        case TokenType::KwU32:
            return TypeInfo(Type::U32, token->lexeme);
        case TokenType::KwU64:
            return TypeInfo(Type::U64, token->lexeme);
        case TokenType::KwF32:
            return TypeInfo(Type::F32, token->lexeme);
        case TokenType::KwF64:
            return TypeInfo(Type::F64, token->lexeme);
        case TokenType::KwBool:
            return TypeInfo(Type::Bool);
        case TokenType::KwString:
            return TypeInfo(Type::String);
        case TokenType::KwVoid:
            return TypeInfo(Type::Void);
        default:
            return std::unexpected(ParseError::UnexpectedToken);
    }
}