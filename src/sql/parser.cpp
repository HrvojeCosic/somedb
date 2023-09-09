#include "../../include/sql/parser.hpp"
#include <iostream>

namespace somedb {

Parser::Parser(Lexer &lexer) : position(-1) { populateTokens(lexer); };

SqlExprRef Parser::parse(uint precedence) {
    auto expr = parsePrefix();

    while (nextPrecedence() > precedence) {
        expr = parseInfix(std::move(expr), nextPrecedence());
    }

    return expr;
};

SqlExprRef Parser::parsePrefix() {
    auto tok = tokens.at(nextTokenPos());

    switch (tok.type) {
    case IDENTIFIER:
    case INT:
        return std::make_unique<SqlIdentifier>(tok.literal);
    case LPAREN: {
        auto expr = parse();
        nextTokenPos(); // consume the right paren
        return expr;
    }
    case KEYWORD:
        if (tok.literal == "SELECT")
            return parseSelectStatement();
        else
            return parseSelectStatement(); // TODO: SUPPORT OTHER KEYWORDS (this is just to avoid a warning)
    default:
        throw std::runtime_error("Unsupported prefix token");
    }
};

SqlExprRef Parser::parseInfix(SqlExprRef left, uint precedence) {
    auto tok = tokens.at(peekTokenPos());

    switch (tok.type) {
    case PLUS:
    case MINUS:
    case ASTERISK:
    case SLASH:
    case EQUALS:
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

std::unique_ptr<SqlSelect> Parser::parseSelectStatement() {
    auto table_name = std::string();
    auto projection = std::vector<SqlExprRef>();
    auto select_tok = tokens.at(position);

    populateProjectionList(projection);

    // Set the table name
    if (tokens.at(nextTokenPos()).literal != "FROM")
        throw std::runtime_error("Invalid SELECT statement near: " + tokens.at(position).literal);
    table_name = tokens.at(nextTokenPos()).literal;

    auto select_statement = std::make_unique<SqlSelect>(table_name, projection);

    // Optionally, set the selection expression
    if (peekTokenPos() != -1 && tokens.at(peekTokenPos()).literal == "WHERE") {
        nextTokenPos(); // consume
        select_statement->selection = parse();
    }

    return select_statement;
};

void Parser::populateProjectionList(std::vector<SqlExprRef> &plist) {
    auto peek = tokens.at(peekTokenPos());
    if (peek.type != ASTERISK && peek.type != IDENTIFIER) {
        throw std::runtime_error("Projection needs to start with identifier or *, but started with " + peek.literal);
    }

    // consume first projection param
    plist.emplace_back(std::make_unique<SqlIdentifier>(tokens.at(nextTokenPos()).literal));
    while (tokens.at(peekTokenPos()).type == COMMA) {
        nextTokenPos(); // consume comma
        plist.emplace_back(std::make_unique<SqlIdentifier>(tokens.at(nextTokenPos()).literal));
    }
};

void Parser::populateTokens(Lexer lexer) {
    std::unique_ptr<Token> tok = std::make_unique<Token>();
    lexer.nextToken(*tok);

    while (tok->type != END) {
        tokens.emplace_back(*tok);
        lexer.nextToken(*tok);
    }
}

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