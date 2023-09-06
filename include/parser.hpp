#pragma once

#include "./expression.hpp"
#include "./lexer.hpp"
#include <optional>

namespace somedb {

struct Parser {
    std::vector<Token> tokens;
    int position;

    Parser(Lexer &lexer) : position(-1) { populateTokens(lexer); };

    /* Parses the expression consisting of tokens */
    std::unique_ptr<SqlExpr> parse(uint precedence = 0);

    /* Parses the next prefix expression from tokens */
    std::unique_ptr<SqlExpr> parsePrefix();

    /* Parses the next infix expression from tokens */
    std::unique_ptr<SqlExpr> parseInfix(std::unique_ptr<SqlExpr> left, uint precedence);

  private:
    /* Calculates the precedence value of the next token */
    uint nextPrecedence();

    /* Consumes next token and returns its position, or -1 if there is no token remaining */
    inline int nextTokenPos() { return position < (int)tokens.size() ? ++position : -1; };

    /* Returns the position of the token ahead (by one) of the current token, or -1 if that would reach out of bounds */
    inline int peekTokenPos() { return position + 1 < (int)tokens.size() ? position + 1 : -1; };

    /* Populates tokens with tokens from current to last token from the lexer (caller takes care of first token
     * position) */
    inline void populateTokens(Lexer lexer) {
        std::unique_ptr<Token> tok = std::make_unique<Token>();
        lexer.nextToken(*tok);

        while (tok->type != END) {
            tokens.emplace_back(*tok);
            lexer.nextToken(*tok);
        }
    }
};
} // namespace somedb