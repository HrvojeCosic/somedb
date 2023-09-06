#pragma once

#include "./shared.h"
#include <algorithm>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>

namespace somedb {

enum TokenType {
    ILLEGAL,
    KEYWORD,
    IDENTIFIER,
    INT,
    STRING,
    EQUALS,
    PLUS,
    MINUS,
    COMMA,
    SEMICOLON,
    ASTERISK,
    SLASH,
    LPAREN,
    RPAREN,
    LBRACE,
    RBRACE,
    LT,
    GT,
    LT_EQ,
    GT_EQ,
    END
};

struct Token {
    TokenType type;
    std::string literal;
    static constexpr std::array<const char *, 8> keywords = {"SELECT", "UPDATE", "DELETE", "INSERT",
                                                             "WHERE",  "FROM",   "AS",     "OR"};

    Token(){};
    Token(TokenType type, std::string literal) : type(type), literal(literal){};

    bool operator==(const Token &other) const { return type == other.type && literal == other.literal; }

    /* Determines whether the string is keyword or an identifier and returns that type */
    constexpr static inline TokenType getStringType(std::string &str) {
        auto find_res = std::find(keywords.begin(), keywords.end(), str);
        return find_res == std::end(keywords) ? IDENTIFIER : KEYWORD;
    }
};

struct Lexer {
    const std::string &input;
    u16 position;
    u16 ahead_position;
    char curr_ch;

    Lexer(const std::string &input);

    /* Returns the token of the current char through the out param TOKEN */
    void nextToken(Token &token);

  private:
    /* Checks if current char is a letter */
    inline bool isLetter() {
        return ((curr_ch >= 'a' && curr_ch <= 'z') || (curr_ch >= 'A' && curr_ch <= 'Z') || curr_ch == '_');
    };

    /* Checks if current char is a digit */
    inline bool isDigit() { return curr_ch >= '0' && curr_ch <= '9'; }

    /* Updates the current char and char pointers if there is anything left to read */
    inline void readChar() {
        curr_ch = ahead_position >= input.length() ? '\0' : input.at(ahead_position);

        position = ahead_position;
        ahead_position++;
    };

    /* Returns the next char if there is one without modifying the lexer state */
    inline char peekNext() { return (ahead_position >= input.size() ? '\0' : input.at(ahead_position)); };

    /* Moves current char past whitespace */
    inline void eatWhitespace() {
        while (curr_ch == ' ' || curr_ch == '\n' || curr_ch == '\t' || curr_ch == '\r')
            readChar();
    };
};

} // namespace somedb