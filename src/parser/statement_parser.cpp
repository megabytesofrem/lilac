#include "../parser.h"

std::expected<NodePtr, ParseError> Parser::parseStatement() {
    auto token = peek();
    if (!token || token->type == TokenType::Eof) {
        return std::unexpected(ParseError::EndOfInput);
    }

    switch (token->type) {
        case TokenType::KwFor:
            return parseForStatement();
        case TokenType::KwForeach:
            return parseForeachStatement();
        case TokenType::KwWhile:
            return parseWhileStatement();
        case TokenType::Symbol:
            if (token->symbol == Symbol::LBrace) {
                return parseBlock();
            }
            break;
        default:
            break;
    }

    auto statement = token->type == TokenType::KwLet ? parseLetStatement()
                                                     : parseExpression();
    assert_expected(statement);

    auto terminator = peek();

    // Check for a semicolon terminator after the statement
    if (terminator && terminator->isSymbol() &&
        terminator->symbol == Symbol::Semicolon) {
        next();
    } else if (!terminator) {
        // No terminator, so end of input
        return std::unexpected(ParseError::EndOfInput);
    } else if (terminator->type != TokenType::Eof &&
               !(terminator->isSymbol() &&
                 terminator->symbol == Symbol::RBrace)) {
        // Unexpected token encountered after the statement, we expected a semicolon,
        // closing brace (to end a block), or end of input
        return std::unexpected(ParseError::UnexpectedToken);
    }

    return statement;
}

/**
 * Parse a block statement, consisting of zero or more statements enclosed in braces.
 * 
 * ```
 * { statement1; statement2; statement...; }
 * ```
 */
std::expected<NodePtr, ParseError> Parser::parseBlock() {
    auto open = expectSymbol(Symbol::LBrace);
    assert_expected(open);

    std::vector<NodePtr> statements;
    while (true) {
        auto token = peek();
        if (!token || token->type == TokenType::Eof) {
            return std::unexpected(ParseError::EndOfInput);
        }

        // Check for the closing brace to end the block
        if (token->isSymbol() && token->symbol == Symbol::RBrace) {
            next();
            return std::make_unique<BlockStmt>(std::move(statements));
        }

        // Parse as many statements as possible until a closing brace is encountered
        auto statement = parseStatement();
        assert_expected(statement);

        statements.push_back(std::move(*statement));
    }
}

/**
 * Parse a let statement.
 * 
 * ```
 * let name: type = value
 * ```
 */
std::expected<NodePtr, ParseError> Parser::parseLetStatement() {
    auto kw = expect(TokenType::KwLet);
    assert_expected(kw);

    auto name = parseIdentifier();
    assert_expected(name);

    std::optional<TypeInfo> type;
    auto token = peek();
    if (token && token->isSymbol() && token->symbol == Symbol::Colon) {
        next();
        auto parsedType = parseType();
        assert_expected(parsedType);
        type = std::move(*parsedType);
    }

    auto eq = expectSymbol(Symbol::Equal);
    assert_expected(eq);

    auto value = parseExpression();
    assert_expected(value);

    // Make the let statement node.
    return std::make_unique<LetStmt>(*name, std::move(*value), std::move(type));
}

/**
 * Parse a C-style for loop.
 * 
 * ```
 * for (let initializer: type = value; condition; increment) {
 *   body
 * }
 * ```
 */
std::expected<NodePtr, ParseError> Parser::parseForStatement() {
    auto kw = expect(TokenType::KwFor);
    assert_expected(kw);
    auto open = expectSymbol(Symbol::LParen);
    assert_expected(open);

    NodePtr initializer;
    NodePtr condition;
    NodePtr increment;

    auto token = peek();
    if (!token || !token->isSymbol() || token->symbol != Symbol::Semicolon) {
        auto parsed = token && token->type == TokenType::KwLet
                          ? parseLetStatement()
                          : parseExpression();
        assert_expected(parsed);
        initializer = std::move(*parsed);
    }
    auto firstSeparator = expectSymbol(Symbol::Semicolon);
    assert_expected(firstSeparator);

    token = peek();
    if (!token || !token->isSymbol() || token->symbol != Symbol::Semicolon) {
        auto parsed = parseExpression();
        assert_expected(parsed);
        condition = std::move(*parsed);
    }
    auto secondSeparator = expectSymbol(Symbol::Semicolon);
    assert_expected(secondSeparator);

    token = peek();
    if (!token || !token->isSymbol() || token->symbol != Symbol::RParen) {
        auto parsed = parseExpression();
        assert_expected(parsed);
        increment = std::move(*parsed);
    }
    auto close = expectSymbol(Symbol::RParen);
    assert_expected(close);

    auto body = parseStatement();
    assert_expected(body);

    // Make the for statement node.
    return std::make_unique<ForStmt>(std::move(initializer),
                                     std::move(condition), std::move(increment),
                                     std::move(*body));
}

/**
 * Parse a modern foreach loop.
 * 
 * ```
 * foreach (name in collection) {
 *   body
 * }
 * ```
 */
std::expected<NodePtr, ParseError> Parser::parseForeachStatement() {
    auto kw = expect(TokenType::KwForeach);
    assert_expected(kw);
    auto open = expectSymbol(Symbol::LParen);
    assert_expected(open);

    auto name = parseIdentifier();
    assert_expected(name);
    auto in = expect(TokenType::KwIn);
    assert_expected(in);
    auto collection = parseExpression();
    assert_expected(collection);

    auto close = expectSymbol(Symbol::RParen);
    assert_expected(close);
    auto body = parseStatement();
    assert_expected(body);

    // Make the foreach statement node.
    return std::make_unique<ForeachStmt>(
        std::move(*name), std::move(*collection), std::move(*body));
}

/**
 * Parse a while loop.
 * 
 * ```
 * while (condition) {
 *   body
 * }
 * ```
 */
std::expected<NodePtr, ParseError> Parser::parseWhileStatement() {
    auto kw = expect(TokenType::KwWhile);
    assert_expected(kw);
    auto open = expectSymbol(Symbol::LParen);
    assert_expected(open);
    auto condition = parseExpression();
    assert_expected(condition);
    auto close = expectSymbol(Symbol::RParen);
    assert_expected(close);

    auto body = parseStatement();
    assert_expected(body);

    // Make the while statement node.
    return std::make_unique<WhileStmt>(std::move(*condition), std::move(*body));
}
