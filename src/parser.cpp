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

std::optional<Token> Parser::peek(size_t offset) { return peekToken(offset); }

std::optional<Token> Parser::next() {
    auto token = peekToken();
    if (token.has_value()) {
        bufferedTokens.pop_front();
    }
    return token;
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

std::expected<Token, ParseError> Parser::expectSymbol(Symbol sym) {
    auto token = next();
    if (!token.has_value()) {
        return std::unexpected(ParseError::EndOfInput);
    }
    if (token->type != TokenType::Symbol || token->symbol != sym) {
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
        case TokenType::Symbol:
            return parseSelectorCall();
        default:
            return std::unexpected(ParseError::UnexpectedToken);
    }
}

std::expected<NodePtr, ParseError> Parser::parseExpression() {
    return parsePrimary();
}

std::expected<NodePtr, ParseError> Parser::parse() { return parseExpression(); }

std::expected<KeywordArgument, ParseError> Parser::parseKeywordArgument() {
    // A keyword argument consists of an identifier followed by a colon and an
    // expression value i.e., `identifier: value`

    auto identifier = expect(TokenType::Ident);
    assert_expected(identifier);

    auto colon = expectSymbol(Symbol::Colon);
    assert_expected(colon);

    auto value = parseExpression();
    assert_expected(value);

    return KeywordArgument(identifier->lexeme, std::move(*value));
}

std::expected<std::vector<KeywordArgument>, ParseError>
Parser::parseKeywordSelector() {
    // A keyword-based selector consists of one or more keyword arguments i.e.,
    // `identifier: value` pairs
    std::vector<KeywordArgument> arguments;

    while (true) {
        auto argument = parseKeywordArgument();
        assert_expected(argument);
        arguments.push_back(std::move(*argument));

        auto nextToken = peek();
        auto followingToken = peek(1);
        if (!nextToken.has_value() || !nextToken->isIdentifier() ||
            !followingToken.has_value() || !followingToken->isSymbol() ||
            followingToken->symbol != Symbol::Colon) {
            break;
        }
    }

    return arguments;
}

std::expected<std::unique_ptr<UnarySelectorExpr>, ParseError>
Parser::parseUnarySelector() {
    // A unary selector consists of an identifier not immediately followed by a
    // colon i.e `identifier` rather than `identifier:`

    auto nextToken = peek(1);
    if (nextToken.has_value() && nextToken->isSymbol() &&
        nextToken->symbol == Symbol::Colon) {
        return std::unexpected(ParseError::UnexpectedToken);
    }

    auto identifier = expect(TokenType::Ident);
    assert_expected(identifier);

    return std::make_unique<UnarySelectorExpr>(identifier->lexeme);
}

std::expected<NodePtr, ParseError> Parser::parseSelectorCall() {
    // An Objective-C style selector call which can either be keyword-based or
    // unary message sends, i.e [target selector:arg] or [target selector]

    auto openBracket = expectSymbol(Symbol::LBracket);
    assert_expected(openBracket);

    auto target = parsePrimary();
    assert_expected(target);

    auto selectorToken = peek();
    if (!selectorToken.has_value() || !selectorToken->isIdentifier()) {
        return std::unexpected(ParseError::UnexpectedToken);
    }

    auto followingToken = peek(1);

    // Check if the following token indicates a keyword-based selector call
    if (followingToken.has_value() && followingToken->isSymbol() &&
        followingToken->symbol == Symbol::Colon) {
        auto keywordArguments = parseKeywordSelector();
        assert_expected(keywordArguments);

        std::vector<ArgumentPair<std::string_view, NodePtr>> arguments;
        std::string selector;
        for (auto& argument : *keywordArguments) {
            selector.append(argument.getKeyword());
            selector.push_back(':');
            arguments.emplace_back(argument.getKeyword(), argument.takeValue());
        }

        auto closeBracket = expectSymbol(Symbol::RBracket);
        assert_expected(closeBracket);

        // Make the selector call node with the target, selector, and arguments
        // and keyword call arity.
        return std::make_unique<SelectorCall>(
            std::move(*target), std::move(selector), std::move(arguments),
            CallArity::Keyword);
    }

    auto unarySelector = parseUnarySelector();
    assert_expected(unarySelector);
    const std::string selector((*unarySelector)->getSelector());

    // If it's not a keyword-based selector call, treat it as a unary message
    // send.
    auto closeBracket = expectSymbol(Symbol::RBracket);
    assert_expected(closeBracket);

    // Make the selector call node with the target, selector, and unary call
    // arity.
    return std::make_unique<SelectorCall>(
        std::move(*target), selector,
        std::vector<ArgumentPair<std::string_view, NodePtr>>{},
        CallArity::Unary);
}