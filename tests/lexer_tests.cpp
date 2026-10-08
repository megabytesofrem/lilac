#include <cstdio>
#include <cstdlib>
#include <string_view>

#include "lexer.h"

static void check(bool condition) {
    if (!condition) {
        std::fprintf(stderr, "Lexer regression check failed.\n");
        std::exit(EXIT_FAILURE);
    }
}

static void expect(Lexer& lexer, TokenType type, std::string_view lexeme) {
    auto peeked = lexer.peek();
    auto repeated = lexer.peek();
    check(peeked.has_value() && repeated.has_value());
    check(peeked->type == type && repeated->type == type);
    check(peeked->lexeme == lexeme && repeated->lexeme == lexeme);
    auto token = lexer.next();
    check(token.has_value());
    check(token->type == type);
    check(token->lexeme == lexeme);
    check(token->length == lexeme.size());
}

int main() {
    Lexer lexer("let value 42 5.0 \"text\" 'x' // end");
    expect(lexer, TokenType::KwLet, "let");
    expect(lexer, TokenType::Whitespace, " ");
    expect(lexer, TokenType::Ident, "value");
    expect(lexer, TokenType::Whitespace, " ");
    expect(lexer, TokenType::Number, "42");
    expect(lexer, TokenType::Whitespace, " ");
    expect(lexer, TokenType::Float, "5.0");
    expect(lexer, TokenType::Whitespace, " ");
    expect(lexer, TokenType::String, "\"text\"");
    expect(lexer, TokenType::Whitespace, " ");
    expect(lexer, TokenType::Char, "'x'");
    expect(lexer, TokenType::Whitespace, " ");
    expect(lexer, TokenType::Comment, "// end");
    expect(lexer, TokenType::Eof, "");
    expect(lexer, TokenType::Eof, "");
    check(lexer.atEnd());

    const char bytes[] = {'a', 'b', '7'};
    Lexer bounded(std::string_view(bytes, sizeof(bytes)));
    expect(bounded, TokenType::Ident, "ab7");
    expect(bounded, TokenType::Eof, "");

    const char number[] = {'1', '2'};
    Lexer boundedNumber(std::string_view(number, sizeof(number)));
    expect(boundedNumber, TokenType::Number, "12");
    expect(boundedNumber, TokenType::Eof, "");

    Lexer sum("2 + 1");
    expect(sum, TokenType::Number, "2");
    expect(sum, TokenType::Whitespace, " ");
    expect(sum, TokenType::Plus, "+");
    expect(sum, TokenType::Whitespace, " ");
    expect(sum, TokenType::Number, "1");
    expect(sum, TokenType::Eof, "");

    Lexer operators("+-*/=!<> <= >= == != (){}[] .. . ,:;");
    expect(operators, TokenType::Plus, "+");
    expect(operators, TokenType::Minus, "-");
    expect(operators, TokenType::Star, "*");
    expect(operators, TokenType::Slash, "/");
    expect(operators, TokenType::Equal, "=");
    expect(operators, TokenType::Bang, "!");
    expect(operators, TokenType::Less, "<");
    expect(operators, TokenType::Greater, ">");
    expect(operators, TokenType::Whitespace, " ");
    expect(operators, TokenType::LessEqual, "<=");
    expect(operators, TokenType::Whitespace, " ");
    expect(operators, TokenType::GreaterEqual, ">=");
    expect(operators, TokenType::Whitespace, " ");
    expect(operators, TokenType::EqualEqual, "==");
    expect(operators, TokenType::Whitespace, " ");
    expect(operators, TokenType::BangEqual, "!=");
    expect(operators, TokenType::Whitespace, " ");
    expect(operators, TokenType::LParen, "(");
    expect(operators, TokenType::RParen, ")");
    expect(operators, TokenType::LBrace, "{");
    expect(operators, TokenType::RBrace, "}");
    expect(operators, TokenType::LBracket, "[");
    expect(operators, TokenType::RBracket, "]");
    expect(operators, TokenType::Whitespace, " ");
    expect(operators, TokenType::DotDot, "..");
    expect(operators, TokenType::Whitespace, " ");
    expect(operators, TokenType::Dot, ".");
    expect(operators, TokenType::Whitespace, " ");
    expect(operators, TokenType::Comma, ",");
    expect(operators, TokenType::Colon, ":");
    expect(operators, TokenType::Semicolon, ";");
    expect(operators, TokenType::Eof, "");

    const char plus[] = {'+'};
    Lexer boundedPlus(std::string_view(plus, sizeof(plus)));
    expect(boundedPlus, TokenType::Plus, "+");
    expect(boundedPlus, TokenType::Eof, "");

    Lexer unknown("@");
    expect(unknown, TokenType::Unknown, "@");
    expect(unknown, TokenType::Eof, "");

    Lexer empty("");
    expect(empty, TokenType::Eof, "");
    Lexer line("//\n");
    expect(line, TokenType::Comment, "//");
    expect(line, TokenType::Whitespace, "\n");
    expect(line, TokenType::Eof, "");

    for (const char* invalid : {"\"", "\"open", "'", "'x", "''", "'ab'"}) {
        Lexer malformed(invalid);
        check(!malformed.peek().has_value());
        check(!malformed.next().has_value());
        check(!malformed.atEnd());
    }

    Token slice(TokenType::Ident, "prefix_suffix", 6);
    check(slice.lexeme == "prefix");
    return EXIT_SUCCESS;
}
