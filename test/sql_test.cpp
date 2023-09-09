#include "../include/sql/lexer.hpp"
#include "../include/sql/parser.hpp"
#include "../include/sql/primitive.hpp"
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

TEST_F(SqlTestFixture, PrimitivesTest) {
    // Boolean tests
    auto bool_type = std::make_shared<BooleanPrimitiveType>();
    auto bool_true = std::make_unique<PrimitiveValue>(bool_type, true);
    auto bool_false = std::make_unique<PrimitiveValue>(bool_type, false);

    EXPECT_EQ(bool_type->equals(*bool_true, *bool_true), CmpState::SQL_TRUE);
    EXPECT_EQ(bool_type->equals(*bool_true, *bool_false), CmpState::SQL_FALSE);
    EXPECT_EQ(bool_type->notEquals(*bool_true, *bool_false), CmpState::SQL_TRUE);
    EXPECT_EQ(bool_type->greaterThan(*bool_true, *bool_false), CmpState::SQL_TRUE);
    EXPECT_EQ(bool_type->lessThan(*bool_false, *bool_true), CmpState::SQL_TRUE);

    // Varchar tests
    std::string varchar = "ABCD EFGH IJK foo";
    std::string varchar_other = "LMNOP QRS TUV bar";
    auto varchar_type = std::make_shared<VarcharPrimitiveType>();
    auto varchar_val1 = std::make_unique<PrimitiveValue>(varchar_type, varchar.data(), varchar.length());
    auto varchar_val2 = std::make_unique<PrimitiveValue>(varchar_type, varchar.data(), varchar.length());
    auto varchar_val3 = std::make_unique<PrimitiveValue>(varchar_type, varchar_other.data(), varchar_other.length());

    EXPECT_EQ(varchar_type->equals(*varchar_val1, *varchar_val2), CmpState::SQL_TRUE);
    EXPECT_EQ(varchar_type->equals(*varchar_val1, *varchar_val3), CmpState::SQL_FALSE);
    EXPECT_EQ(varchar_type->notEquals(*varchar_val1, *varchar_val3), CmpState::SQL_TRUE);
    EXPECT_EQ(varchar_type->greaterThan(*varchar_val3, *varchar_val1), CmpState::SQL_TRUE);
    EXPECT_EQ(varchar_type->lessThan(*varchar_val1, *varchar_val3), CmpState::SQL_TRUE);

    // Integer tests
    auto int_type = std::make_shared<IntegerPrimitiveType>();
    auto int_val1 = std::make_unique<PrimitiveValue>(int_type, 42);
    auto int_val2 = std::make_unique<PrimitiveValue>(int_type, 42);
    auto int_val3 = std::make_unique<PrimitiveValue>(int_type, 100);

    EXPECT_EQ(int_type->equals(*int_val1, *int_val2), CmpState::SQL_TRUE);
    EXPECT_EQ(int_type->equals(*int_val1, *int_val3), CmpState::SQL_FALSE);
    EXPECT_EQ(int_type->notEquals(*int_val1, *int_val3), CmpState::SQL_TRUE);
    EXPECT_EQ(int_type->greaterThan(*int_val3, *int_val1), CmpState::SQL_TRUE);
    EXPECT_EQ(int_type->lessThan(*int_val1, *int_val3), CmpState::SQL_TRUE);

    // Integer arithmetic tests
    auto int_result = int_type->add(*int_val1, *int_val3);
    EXPECT_EQ(int_result.value.integer, 142);
    int_result = int_type->subtract(*int_val1, *int_val3);
    EXPECT_EQ(int_result.value.integer, -58);
    int_result = int_type->multiply(*int_val1, *int_val3);
    EXPECT_EQ(int_result.value.integer, 4200);
    int_result = int_type->divide(*int_val3, *int_val1);
    EXPECT_EQ(int_result.value.integer, 2);

    // Decimal tests
    auto decimal_type = std::make_shared<DecimalPrimitiveType>();
    auto decimal_val1 = std::make_unique<PrimitiveValue>(decimal_type, 42.5);
    auto decimal_val2 = std::make_unique<PrimitiveValue>(decimal_type, 42.5);
    auto decimal_val3 = std::make_unique<PrimitiveValue>(decimal_type, 100.0);

    EXPECT_EQ(decimal_type->equals(*decimal_val1, *decimal_val2), CmpState::SQL_TRUE);
    EXPECT_EQ(decimal_type->equals(*decimal_val1, *decimal_val3), CmpState::SQL_FALSE);
    EXPECT_EQ(decimal_type->notEquals(*decimal_val1, *decimal_val3), CmpState::SQL_TRUE);
    EXPECT_EQ(decimal_type->greaterThan(*decimal_val3, *decimal_val1), CmpState::SQL_TRUE);
    EXPECT_EQ(decimal_type->lessThan(*decimal_val1, *decimal_val3), CmpState::SQL_TRUE);

    // Decimal arithmetic tests
    auto decimal_result = decimal_type->add(*decimal_val1, *decimal_val3);
    EXPECT_DOUBLE_EQ(decimal_result.value.decimal, 142.5);
    decimal_result = decimal_type->subtract(*decimal_val1, *decimal_val3);
    EXPECT_DOUBLE_EQ(decimal_result.value.decimal, -57.5);
    decimal_result = decimal_type->multiply(*decimal_val1, *decimal_val3);
    EXPECT_DOUBLE_EQ(decimal_result.value.decimal, 4250.0);
    decimal_result = decimal_type->divide(*decimal_val3, *decimal_val1);
    EXPECT_DOUBLE_EQ(decimal_result.value.decimal, 2.3529411764705883);
}

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