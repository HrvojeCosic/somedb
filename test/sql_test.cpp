#include "../include/lexer.hpp"
#include <gtest/gtest.h>

namespace somedb {

class SqlTestFixture : public testing::Test {
  protected:
    void SetUp() override {}

    void TearDown() override {}
};

TEST_F(SqlTestFixture, TokenTest) {
    const std::string input = "SELECT * FROM some_table WHERE some_col = 123;";
    Lexer lexer(input);

    Token *t1 = new Token();
    Token *t2 = new Token();
    Token *t3 = new Token();
    Token *t4 = new Token();
    Token *t5 = new Token();
    Token *t6 = new Token();
    Token *t7 = new Token();
    Token *t8 = new Token();
    Token *t9 = new Token();
    Token *t10 = new Token();

    lexer.nextToken(*t1);
    lexer.nextToken(*t2);
    lexer.nextToken(*t3);
    lexer.nextToken(*t4);
    lexer.nextToken(*t5);
    lexer.nextToken(*t6);
    lexer.nextToken(*t7);
    lexer.nextToken(*t8);
    lexer.nextToken(*t9);
    lexer.nextToken(*t10);

    EXPECT_EQ(*t1, Token(KEYWORD, "SELECT"));
    EXPECT_EQ(*t2, Token(ASTERISK, "*"));
    EXPECT_EQ(*t3, Token(KEYWORD, "FROM"));
    EXPECT_EQ(*t4, Token(IDENTIFIER, "some_table"));
    EXPECT_EQ(*t5, Token(KEYWORD, "WHERE"));
    EXPECT_EQ(*t6, Token(IDENTIFIER, "some_col"));
    EXPECT_EQ(*t7, Token(ASSIGN, "="));
    EXPECT_EQ(*t8, Token(INT, "123"));
    EXPECT_EQ(*t9, Token(SEMICOLON, ";"));
    EXPECT_EQ(t10->type, END);
}
} // namespace somedb