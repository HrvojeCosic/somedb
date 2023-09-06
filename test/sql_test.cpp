#include "../include/expression.hpp"
#include "../include/lexer.hpp"
#include "../include/parser.hpp"
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

    inline void TestSqlBinaryExpr(std::unique_ptr<SqlExpr> &parsed, const std::string &exp_left,
                                  const std::string &exp_op, const std::string &exp_right) {
        auto &actual = dynamic_cast<SqlBinaryExpr &>(*parsed);

        EXPECT_EQ(actual.left->toString(), exp_left);
        EXPECT_EQ(actual.op, exp_op);
        EXPECT_EQ(actual.right->toString(), exp_right);
    };

    inline std::unique_ptr<SqlExpr> parse(std::string input) {
        Lexer lexer(input);
        Parser parser(lexer);
        return parser.parse();
    };
};

TEST_F(SqlTestFixture, LexTest) {
    std::string input = "SELECT * FROM some_table WHERE some_col = 123 OR some_other_col <= 321;";
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

TEST_F(SqlTestFixture, ParseTest) {
    auto in1 = "1 + 2 * 3";
    auto out1 = parse(in1);
    TestSqlBinaryExpr(out1, "1", "+", "2*3");

    auto in2 = "1 * 2 + 3";
    auto out2 = parse(in2);
    TestSqlBinaryExpr(out2, "1*2", "+", "3");
}
} // namespace somedb