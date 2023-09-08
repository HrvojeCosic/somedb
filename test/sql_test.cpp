#include "../include/sql/expression.hpp"
#include "../include/sql/lexer.hpp"
#include "../include/sql/parser.hpp"
#include <array>
#include <gtest/gtest.h>
#include <memory>

namespace somedb {

class SqlTestFixture : public testing::Test {
  protected:
    void SetUp() override {}

    void TearDown() override {}

    void TestNextToken(Token expected, Lexer &lexer) {
        std::unique_ptr<Token> actual = std::make_unique<Token>();
        lexer.nextToken(*actual);
        EXPECT_EQ(expected, *actual);
    };

    void TestSqlBinaryExpr(SqlExprRef &parsed, const std::string &exp_left, const std::string &exp_op,
                           const std::string &exp_right) {
        auto &actual = dynamic_cast<SqlBinaryExpr &>(*parsed);

        EXPECT_EQ(actual.left->toString(), exp_left);
        EXPECT_EQ(actual.op, exp_op);
        EXPECT_EQ(actual.right->toString(), exp_right);
    };

    void TestSelectStatement(SqlExprRef &parsed, std::vector<std::string> exp_projection, std::string exp_tab_name,
                             std::string exp_selection) {
        auto &actual = dynamic_cast<SqlSelect &>(*parsed);

        EXPECT_EQ(actual.table_name, exp_tab_name);

        // dont compare if there is no selection specified
        if (actual.selection.has_value()) {
            EXPECT_EQ(actual.selection->get()->toString(), exp_selection);
        }

        for (uint i = 0; i < actual.projection.size(); i++)
            EXPECT_EQ(actual.projection.at(i)->toString(), exp_projection.at(i));
    };

    SqlExprRef parse(std::string input) {
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
    auto out1 = parse("1 + 2 * 3");
    TestSqlBinaryExpr(out1, "1", "+", "2*3");

    auto out2 = parse("1 * 2 + 3");
    TestSqlBinaryExpr(out2, "1*2", "+", "3");

    auto out3 = parse("1 * (2 + 3)");
    TestSqlBinaryExpr(out3, "1", "*", "2+3");

    auto out4 = parse("SELECT * FROM foo");
    TestSelectStatement(out4, std::vector<std::string>{"*"}, "foo", "");

    auto out5 = parse("SELECT some_col, other_col FROM foo WHERE some_col <= 10");
    TestSelectStatement(out5, std::vector<std::string>{"some_col", "other_col"}, "foo", "some_col<=10");
}
} // namespace somedb