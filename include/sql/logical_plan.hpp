#pragma once

#include "./access_method.hpp"
#include "./logical_expression.hpp"
#include "./primitive.hpp"
#include "./schema.hpp"

#include <iostream>
#include <memory>
#include <vector>

namespace somedb {

struct LogicalOperator;
using LogicalOperatorRef = std::unique_ptr<LogicalOperator>;

struct LogicalOperator {
    LogicalOperator(LogicalOperatorRef in) : input(std::move(in)){};

    LogicalOperator(LogicalOperator &&other) noexcept : input(std::move(other.input)) {}

    virtual ~LogicalOperator() = default;

    // Gets the schema of the logical plan operator
    virtual Table schema() const = 0;

    // Gets the children of the logical plan operator
    virtual std::vector<LogicalOperatorRef> children();
    // Returns the plan of the operator as a formatted string
    virtual std::string toString() const = 0;

    // Prints the logical plan from the top (starting from OP) in a readable format (helps in debugging)
    static void pretty(LogicalOperatorRef op, int indent);

    LogicalOperatorRef input;
};

struct LogicalScan : LogicalOperator {
    AccessMethodRef access_method;

    LogicalScan(AccessMethodRef am) : LogicalOperator(nullptr), access_method(std::move(am)){};

    Table schema() const override { return access_method->schema(); };

    std::vector<LogicalOperatorRef> children() override { return std::vector<LogicalOperatorRef>(); };

    std::string toString() const override { return "Scan: " + access_method->schema().name; };
};

struct LogicalSelection : LogicalOperator {
    LogicalExprRef expr;

    LogicalSelection(LogicalOperatorRef in, LogicalExprRef expr)
        : LogicalOperator(std::move(in)), expr(std::move(expr)){};

    Table schema() const override { return input->schema(); };

    std::string toString() const override { return "Select: " + expr->toString(); };
};

// TODO: projection currently only works with column logical expressions,
// but it should work with other logical expressions. For example, it should be possible
// to make a query like: SELECT foo * 100 FROM ...
struct LogicalProjection : LogicalOperator {
    std::vector<ColumnLogicalExpr> proj_cols;

    LogicalProjection(LogicalOperatorRef in, std::vector<ColumnLogicalExpr> proj_cols)
        : LogicalOperator(std::move(in)), proj_cols(std::move(proj_cols)){};

    Table schema() const override;

    std::string toString() const override;
};

enum AggregateOperator { COUNT, AVG, SUM, MIN, MAX };

// TODO: similarly to logical projection, we're currently only working with column expressions for simplicity.
struct LogicalAggregation : LogicalOperator {
    ColumnLogicalExpr aggr_col;
    ColumnLogicalExpr group_col;
    AggregateOperator agg_op;

    LogicalAggregation(LogicalOperatorRef in, ColumnLogicalExpr &ac, ColumnLogicalExpr &gc, AggregateOperator agop)
        : LogicalOperator(std::move(in)), aggr_col(ac), group_col(gc), agg_op(agop){};

    Table schema() const override {
        return Table(input->schema().name,
                     {Column(aggr_col.name, aggr_col.return_type), Column(group_col.name, group_col.return_type)});
    };

    std::string toString() const override {
        return "Aggregate: groupCol: " + group_col.name + " aggregateCol: " + aggr_col.name;
    };
};
} // namespace somedb