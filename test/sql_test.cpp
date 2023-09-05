#include "../include/lexer.hpp"
#include <gtest/gtest.h>
#include <memory>

namespace somedb {

class SqlTestFixture : public testing::Test {
  protected:
    void SetUp() override {}

    void TearDown() override {}

    inline void TestNextToken(Token expected, Lexer &lexer) {
      std::unique_ptr<Token> actual = std::make_unique<Token>();
      lexer.nextToken(*actual);
      EXPECT_EQ(expected, *actual);
    };
};

TEST_F(SqlTestFixture, TokenTest) {
    const std::string input = "SELECT * FROM some_table WHERE some_col = 123 OR some_other_col <= 321;";
    Lexer lexer(input);
    TestNextToken(Token(KEYWORD, "SELECT"), lexer);
    TestNextToken(Token(ASTERISK, "*"), lexer);
    TestNextToken(Token(KEYWORD, "FROM"), lexer);
    TestNextToken(Token(IDENTIFIER, "some_table"), lexer);
    TestNextToken(Token(KEYWORD, "WHERE"), lexer);
    TestNextToken(Token(IDENTIFIER, "some_col"), lexer);
    TestNextToken(Token(EQUALS, "="), lexer);
    TestNextToken(Token(INT, "123"), lexer);
    TestNextToken(Token(KEYWORD, "OR"), lexer);
    TestNextToken(Token(IDENTIFIER, "some_other_col"), lexer);
    TestNextToken(Token(LT_EQ, "<="), lexer);
    TestNextToken(Token(INT, "321"), lexer);
    TestNextToken(Token(SEMICOLON, ";"), lexer);
    TestNextToken(Token(END, "\0"), lexer);
}
} // namespace somedb