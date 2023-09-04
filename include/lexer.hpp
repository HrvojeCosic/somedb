#include "./shared.h"
#include <functional>
#include <string>
#include <unordered_map>

namespace somedb {

enum TokenType {
    ILLEGAL,
    KEYWORD,
    IDENTIFIER,
    INT,
    STRING,
    ASSIGN,
    PLUS,
    MINUS,
    COMMA,
    SEMICOLON,
    ASTERISK,
    LPAREN,
    RPAREN,
    LBRACE,
    RBRACE,
    END
};

struct Token {
    TokenType type;
    std::string literal;

    Token(){};
    Token(TokenType type, std::string literal) : type(type), literal(literal){};

    bool operator==(const Token &other) const { return type == other.type && literal == other.literal; }

    /* Determines whether the string is keyword or an identifier and returns that type */
    static inline TokenType getStringType(std::string &str) {
        std::unordered_map<std::string, bool> keywords = {{"SELECT", 1}, {"UPDATE", 1}, {"DELETE", 1}, {"INSERT", 1},
                                                          {"WHERE", 1},  {"FROM", 1},   {"AS", 1}};
        return keywords.contains(str) ? KEYWORD : IDENTIFIER;
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

    /* Moves current char past whitespace */
    inline void eatWhitespace() {
        while (curr_ch == ' ' || curr_ch == '\n' || curr_ch == '\t' || curr_ch == '\r')
            readChar();
    }
};

} // namespace somedb