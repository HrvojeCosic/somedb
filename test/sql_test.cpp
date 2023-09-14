#include "../include/disk/heapfile.h"
#include "../include/sql/lexer.hpp"
#include "../include/sql/logical_plan.hpp"
#include "../include/sql/parser.hpp"
#include "../include/sql/primitive.hpp"
#include <array>
#include <gtest/gtest.h>
#include <memory>

namespace somedb {

class SqlTestFixture : public testing::Test {
  protected:
    void SetUp() override {}

    void TearDown() override { remove_table("test_table"); }

    void SetupTable() {
        char x[4] = "foo";
        char y[4] = "bar";
        ::Column cols[2] = {{.name_len = 3, .name = x, .type = ColumnType::STRING}, {.name_len = 3, .name = y, .type = DECIMAL}};
        const char *col_names[2] = {"foo", "bar"};
        ColumnType col_types[2] = {ColumnType::STRING, DECIMAL};
        ColumnValue col_vals[2] = {{.string = "Baz"}, {.decimal = 123.12}};
        TuplePtr *t_ptr = (TuplePtr *)malloc(sizeof(TuplePtr));
        DiskManager *disk_mgr = create_table("test_table", cols, 2);
        new_heap_page(disk_mgr);
        AddTupleArgs t_args = {.disk_manager = disk_mgr,
                               .column_names = col_names,
                               .column_values = col_vals,
                               .column_types = col_types,
                               .num_columns = 1,
                               .tup_ptr_out = t_ptr};
        add_tuple(&t_args);
    };

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

// -----------------------------------------------------------------------------------------------------
// TYPES
// -----------------------------------------------------------------------------------------------------
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

    // String tests
    std::string string = "ABCD EFGH IJK foo";
    std::string varchar_other = "LMNOP QRS TUV bar";
    auto varchar_type = std::make_shared<VarcharPrimitiveType>();
    auto varchar_val1 = std::make_unique<PrimitiveValue>(varchar_type, string.data(), string.length());
    auto varchar_val2 = std::make_unique<PrimitiveValue>(varchar_type, string.data(), string.length());
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

// -----------------------------------------------------------------------------------------------------
// LEXER/PARSER
// -----------------------------------------------------------------------------------------------------
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

// -----------------------------------------------------------------------------------------------------
// LOGICAL OPERATORS
// -----------------------------------------------------------------------------------------------------
TEST_F(SqlTestFixture, LogicalOperatorTest) {
    SetupTable();
    AccessMethodRef heapfile_acc = std::make_unique<HeapfileAccess>("test_table");

    // SCAN
    LogicalScan scan(std::move(heapfile_acc));
    Table actual_scan_schema = scan.schema();

    auto type_foo = dynamic_cast<VarcharPrimitiveType &>(*actual_scan_schema.columns[0].data_type);
    auto type_bar = dynamic_cast<DecimalPrimitiveType &>(*actual_scan_schema.columns[1].data_type);
    Column col_foo("foo", std::make_unique<VarcharPrimitiveType>(type_foo));
    Column col_bar("bar", std::make_unique<DecimalPrimitiveType>(type_bar));

    Table expected_schema(actual_scan_schema.name, std::vector<Column>{col_foo, col_bar});
    EXPECT_EQ(expected_schema == actual_scan_schema, true); // NOLINT

    // PROJECT
    auto project_cols =
        std::vector<ColumnLogicalExpr>{ColumnLogicalExpr("foo", std::make_unique<VarcharPrimitiveType>())};
    LogicalProjection project(std::make_unique<LogicalScan>(std::move(scan)), project_cols);
    Table actual_project_schema = project.schema();

    Table expected_project_schema(actual_project_schema.name, std::vector<Column>{col_foo});
    EXPECT_EQ(expected_project_schema == actual_project_schema, true); // NOLINT

    // AGGREGATE
    auto aggr_col = ColumnLogicalExpr("foo", std::make_unique<VarcharPrimitiveType>());
    auto group_col = ColumnLogicalExpr("foo", std::make_unique<VarcharPrimitiveType>());
    LogicalAggregation aggr(std::make_unique<LogicalProjection>(std::move(project)), aggr_col, group_col, SUM);
    Table actual_aggr_schema = aggr.schema();

    Table expected_aggr_schema(actual_aggr_schema.name, std::vector<Column>{col_foo, col_foo});
    EXPECT_EQ(expected_aggr_schema == actual_aggr_schema, true); // NOLINT
}

} // namespace somedb