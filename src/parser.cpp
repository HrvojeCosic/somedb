#include "../include/parser.hpp"
#include "../include/expression.hpp"

namespace somedb {

std::unique_ptr<SqlExpr> Parser::parse(uint precedence) {
    auto expr = parsePrefix();

    while (nextPrecedence() > precedence) {
        expr = parseInfix(std::move(expr), nextPrecedence());
    }

    return expr;
};

std::unique_ptr<SqlExpr> Parser::parsePrefix() {
    auto tok = tokens.at(nextTokenPos());

    switch (tok.type) {
    case INT:
        return std::make_unique<SqlIdentifier>(tok.literal);
    default:
        throw std::runtime_error("Unsupported prefix token");
    }
};

std::unique_ptr<SqlExpr> Parser::parseInfix(std::unique_ptr<SqlExpr> left, uint precedence) {
    auto tok = tokens.at(peekTokenPos());

    switch (tok.type) {
    case PLUS:
    case MINUS:
    case ASTERISK:
    case SLASH:
    case LT:
    case GT:
    case GT_EQ:
    case LT_EQ:
        nextTokenPos(); // consume tok
        return std::make_unique<SqlBinaryExpr>(std::move(left), tok.literal, parse(precedence));
    default:
        throw std::runtime_error("Unsupported infix token");
    }
};

uint Parser::nextPrecedence() {
    auto pos = peekTokenPos();
    if (pos == -1)
        return 0;
    auto next_tok = tokens.at(pos);

    switch (next_tok.type) {
    case EQUALS:
    case GT_EQ:
    case LT_EQ:
    case LT:
    case GT:
        return 1;

    case PLUS:
    case MINUS:
        return 2;

    case SLASH:
    case ASTERISK:
        return 3;

    case LPAREN:
        return 4;

    default:
        return 0;
    }
}
} // namespace somedb