#pragma once

#include "./lexer.hpp"
#include "./primitive.hpp"
#include "./sql_expression.hpp"
#include <optional>

namespace somedb {

struct Parser {
    std::vector<Token> tokens;
    int position;

    Parser(Lexer &lexer);

    /* Parses the expression consisting of tokens */
    SqlExprRef parse(uint precedence = 0);

    /* Parses the next prefix expression from tokens */
    SqlExprRef parsePrefix();

    /* Parses the next infix expression from tokens */
    SqlExprRef parseInfix(SqlExprRef left, uint precedence);

    /* Parses the select statement */
    std::unique_ptr<SqlSelect> parseSelectStatement();

  private:
    /* Calculates the precedence value of the next token */
    uint nextPrecedence();

    /* Populates the given projection list from current place in the token stream */
    void populateProjectionList(std::vector<SqlExprRef> &plist);

    /* Consumes next token and returns its position, or -1 if there is no token remaining */
    inline int nextTokenPos() { return position < (int)tokens.size() ? ++position : -1; };

    /* Returns the position of the token ahead (by one) of the current token, or -1 if that would reach out of bounds */
    inline int peekTokenPos() { return position + 1 < (int)tokens.size() ? position + 1 : -1; };

    /* Populates tokens with tokens from current to last token from the lexer (caller takes care of first token
     * position) */
    void populateTokens(Lexer lexer);
};
} // namespace somedb