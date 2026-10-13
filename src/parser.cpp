#include "parser.h"

#include <memory>
#include <utility>

std::optional<Token> Parser::peek() const { return lexer.peek(); }

std::optional<Token> Parser::next() { return lexer.next(); }

std::expected<Token, ParseError> Parser::look(TokenType type) {
    auto token = peek();
    if (!token.has_value()) {
        return std::unexpected(ParseError::EndOfInput);
    }
    if (token->type != type) {
        return std::unexpected(ParseError::UnexpectedToken);
    }
    return *token;
}

std::expected<Token, ParseError> Parser::expect(TokenType type) {
    auto token = next();
    if (!token.has_value()) {
        return std::unexpected(ParseError::EndOfInput);
    }
    if (token->type != type) {
        return std::unexpected(ParseError::UnexpectedToken);
    }
    return *token;
}

// MARK: Parser Methods

std::expected<NodePtr, ParseError> Parser::parseIntegerLiteral() {
    auto token = expect(TokenType::Number);
    assert_expected(token);

    // Convert lexeme to integer value
    int value = std::stoi(token->lexeme.data());
    return std::make_unique<LiteralExpr>(value);
}

std::expected<NodePtr, ParseError> Parser::parseFloatLiteral() {
    auto token = expect(TokenType::Float);
    assert_expected(token);

    // Convert lexeme to float value
    float value = std::stof(token->lexeme.data());
    return std::make_unique<LiteralExpr>(value);
}

std::expected<NodePtr, ParseError> Parser::parseStringLiteral() {
    auto token = expect(TokenType::String);
    assert_expected(token);

    return std::make_unique<LiteralExpr>(token->lexeme);
}

std::expected<NodePtr, ParseError> Parser::parseIdentifier() {
    auto token = expect(TokenType::Ident);
    assert_expected(token);

    return std::make_unique<IdentifierExpr>(token->lexeme);
}

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
        case TokenType::LBracket:
            return parseSelectorCall();
        default:
            return std::unexpected(ParseError::UnexpectedToken);
    }
}

std::expected<NodePtr, ParseError> Parser::parseExpression() {
    return parseBinaryChain();
}

std::expected<KeywordArgument, ParseError> Parser::parseKeywordArgument() {
    // A keyword argument consists of an identifier followed by a colon and an
    // expression value i.e., `identifier: value`

    auto identifier = expect(TokenType::Ident);
    assert_expected(identifier);

    auto colon = expect(TokenType::Colon);
    assert_expected(colon);

    auto value = parseExpression();
    assert_expected(value);

    return KeywordArgument(identifier->lexeme, std::move(*value));
}

std::expected<NodePtr, ParseError> Parser::parseUnarySelector() {
    // A unary selector consists of an identifier not immediately followed by a
    // colon i.e `identifier` rather than `identifier:`

    auto identifier = expect(TokenType::Ident);
    assert_expected(identifier);

    auto colon = look(TokenType::Colon);
    assert_not_expected(colon);

    return std::make_unique<UnarySelectorExpr>(identifier->lexeme);
}

std::expected<NodePtr, ParseError> Parser::parseUnaryChain() {
    // An Objective-C style unary message chain consists of one or more unary
    // messages sent to a target, i.e., `target identifier1 identifier2 ...`

    auto target = parsePrimary();
    assert_expected(target);

    std::vector<NodePtr> chain;
    while (true) {
        auto token = peek();
        if (!token.has_value() || token->type != TokenType::Ident) {
            break;
        }
        auto unary = parseUnarySelector();
        if (!unary.has_value()) {
            break;
        }
        chain.push_back(std::move(*unary));
    }

    // Construct a unary chain node or return the target if no unary messages
    // were found
    if (chain.empty()) {
        return target;
    }

    // Make the chain with the target as the receiver and the collected unary
    // messages as the elements.
    return std::make_unique<UnaryChain>(
        std::move(*target), std::move(chain), CallArity::Unary);
}

std::expected<NodePtr, ParseError> Parser::parseBinarySelector() {
    auto token = peek();
    if (!token.has_value()) {
        return std::unexpected(ParseError::EndOfInput);
    }

    switch (token->type) {
        case TokenType::Plus:
        case TokenType::Minus:
        case TokenType::Star:
        case TokenType::Slash:
        case TokenType::Equal:
        case TokenType::EqualEqual:
        case TokenType::BangEqual:
        case TokenType::Less:
        case TokenType::LessEqual:
        case TokenType::Greater:
        case TokenType::GreaterEqual:
            break;
        default:
            return std::unexpected(ParseError::UnexpectedToken);
    }

    auto op = next();
    auto value = parseUnaryChain();
    assert_expected(value);

    return std::make_unique<BinarySelectorExpr>(op->lexeme, std::move(*value));
}

std::expected<NodePtr, ParseError> Parser::parseBinaryChain() {
    // An Objective-C style binary message chain consists of one or more binary
    // messages sent to a target, i.e., `target op1 value1 op2 value2 ...`

    auto target = parseUnaryChain();
    assert_expected(target);

    std::vector<NodePtr> chain;
    while (true) {
        auto token = peek();
        if (!token.has_value()) {
            break;
        }
        switch (token->type) {
            case TokenType::Plus:
            case TokenType::Minus:
            case TokenType::Star:
            case TokenType::Slash:
            case TokenType::Equal:
            case TokenType::EqualEqual:
            case TokenType::BangEqual:
            case TokenType::Less:
            case TokenType::LessEqual:
            case TokenType::Greater:
            case TokenType::GreaterEqual:
                break;
            default:
                return chain.empty()
                           ? std::expected<NodePtr, ParseError>(
                                 std::move(*target))
                           : std::expected<NodePtr, ParseError>(
                                 std::make_unique<BinaryChain>(
                                     std::move(*target), std::move(chain),
                                     CallArity::Binary));
        }
        auto binary = parseBinarySelector();
        assert_expected(binary);
        chain.push_back(std::move(*binary));
    }

    // Construct a binary chain node or return the target if no binary messages
    // were found
    if (chain.empty()) {
        return target;
    }

    // Make the chain with the first operator as the selector and the rest of
    // the binary messages as the elements.
    return std::make_unique<BinaryChain>(
        std::move(*target), std::move(chain), CallArity::Binary);
}

std::expected<NodePtr, ParseError> Parser::parseSelectorCall() {
    auto openBracket = expect(TokenType::LBracket);
    assert_expected(openBracket);

    auto target = parsePrimary();
    assert_expected(target);

    auto selector = expect(TokenType::Ident);
    assert_expected(selector);

    auto closeBracket = expect(TokenType::RBracket);
    assert_expected(closeBracket);

    return std::make_unique<SelectorCall>(std::move(*target), selector->lexeme,
                                          std::vector<ArgumentPair<
                                              std::string_view, NodePtr>>{},
                                          CallArity::Unary);
}