#include "../include/lexer.hpp"
#include <string>

namespace somedb {

Lexer::Lexer(const std::string &input) : input(input), position(-1), ahead_position(0), curr_ch('\0') { readChar(); }

void Lexer::nextToken(Token &token) {
    eatWhitespace();

    switch (curr_ch) {
    case '=':
        token.type = ASSIGN;
        token.literal = curr_ch;
        break;
    case '+':
        token.type = PLUS;
        token.literal = curr_ch;
        break;
    case '-':
        token.type = MINUS;
        token.literal = curr_ch;
        break;
    case ',':
        token.type = COMMA;
        token.literal = curr_ch;
        break;
    case ';':
        token.type = SEMICOLON;
        token.literal = curr_ch;
        break;
    case '*':
        token.type = ASTERISK;
        token.literal = curr_ch;
        break;
    case '(':
        token.type = LPAREN;
        token.literal = curr_ch;
        break;
    case ')':
        token.type = RPAREN;
        token.literal = curr_ch;
        break;
    case '{':
        token.type = LBRACE;
        token.literal = curr_ch;
        break;
    case '}':
        token.type = RBRACE;
        token.literal = curr_ch;
        break;
    case '\0':
        token.type = END;
        token.literal = curr_ch;
        break;
    default:
        if (isLetter()) {
            std::string str = "";
            while (isLetter()) {
                str.append(std::string(1, curr_ch));
                readChar();
            }
            token.literal = str;
            token.type = Token::getStringType(str);
            return;
        } else if (isDigit()) {
            std::string num = "";
            while (isDigit()) {
                num.append(std::string(1, curr_ch));
                readChar();
            }
            token.literal = num;
            token.type = INT; // TODO: support other number types
            return;
        } else {
            token.type = ILLEGAL;
        }
        break;
    }

    readChar();
}

} // namespace somedb