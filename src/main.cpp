#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <optional>

#include "lexer.h"

int main(void) {
    std::unique_ptr<Lexer> lexer =
        std::make_unique<Lexer>("2 + 1 \"hello world\"");

    std::optional<Token> token;
    while ((token = lexer->next()).has_value()) {
        if (token->type == TokenType::Eof) {
            break;
        }
        std::cout << "Token type: " << token->tokenTypeToString()
                  << ", lexeme: '" << token->lexeme << "'"
                  << ", length: " << token->length << '\n';
    }
    if (!token.has_value()) {
        std::fprintf(stderr, "Invalid or unterminated literal.\n");
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}