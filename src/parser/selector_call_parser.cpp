#include "../parser.h"

/**
 * Parse a keyword argument from the input stream.
 * 
 * A keyword argument consits of an identifier followed by a colon and an expression value i.e
 * `identifier: value`
 */
std::expected<KeywordArgument, ParseError> Parser::parseKeywordArgument() {
    auto identifier = parseIdentifier();
    assert_expected(identifier);

    auto colon = expectSymbol(Symbol::Colon);
    assert_expected(colon);

    auto value = parseExpression();
    assert_expected(value);

    return KeywordArgument(*identifier, std::move(*value));
}

/**
 * Parse a keyword-based selector from the input stream.
 * 
 * A keyword-based selector consists of one or more keyword arguments i.e.,
 * `identifier: value` pairs.
 */
std::expected<std::vector<KeywordArgument>, ParseError>
Parser::parseKeywordSel() {
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

/**
 * Parse a unary selector from the input stream.
 * 
 * A unary selector consists of an identifier not immediately followed by a
 * colon i.e `identifier` rather than `identifier:`
 */
std::expected<std::unique_ptr<UnarySelector>, ParseError>
Parser::parseUnarySel() {
    auto nextToken = peek(1);
    if (nextToken.has_value() && nextToken->isSymbol() &&
        nextToken->symbol == Symbol::Colon) {
        return std::unexpected(ParseError::UnexpectedToken);
    }

    auto identifier = parseIdentifier();
    assert_expected(identifier);

    return std::make_unique<UnarySelector>(*identifier);
}

/**
 * Parse a selector call from the input stream.
 * 
 * A selector call can either be keyword-based or unary, i.e., `[target selector:arg]` or
 * `[target selector]`.
 * 
*/
std::expected<NodePtr, ParseError> Parser::parseSelectorCall() {
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
        auto keywordArguments = parseKeywordSel();
        assert_expected(keywordArguments);

        std::vector<ArgumentPair<std::string, NodePtr>> arguments;
        std::string selector;
        for (auto& argument : *keywordArguments) {
            selector.append(argument.getKeyword());
            selector.push_back(':');
            arguments.emplace_back(std::string(argument.getKeyword()),
                                   argument.takeValue());
        }

        auto closeBracket = expectSymbol(Symbol::RBracket);
        assert_expected(closeBracket);

        // Make the selector call node with the target, selector, and arguments
        // and keyword call arity.
        return std::make_unique<SelectorCall>(
            std::move(*target), std::move(selector), std::move(arguments),
            CallArity::Keyword);
    }

    auto unarySelector = parseUnarySel();
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
        std::vector<ArgumentPair<std::string, NodePtr>>{},
        CallArity::Unary);
}