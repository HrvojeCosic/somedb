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
    virtual ~LogicalOperator() = default;

    // Gets the schema of the logical plan operator
    virtual Table schema() const = 0;

    // Gets the children of the logical plan operator
    virtual std::vector<LogicalOperatorRef> children() = 0;

    // Returns the plan of the operator as a formatted string
    virtual std::string toString() const = 0;

    // Prints the logical plan from the top (starting from OP) in a readable format (helps in debugging)
    static void pretty(LogicalOperatorRef op, int indent);
};

struct LogicalScan : LogicalOperator {
    AccessMethodRef access_method;

    LogicalScan(AccessMethodRef access_method) : access_method(std::move(access_method)){};

    Table schema() const override { return access_method->schema(); };

    std::vector<LogicalOperatorRef> children() override { return std::vector<LogicalOperatorRef>(); };

    std::string toString() const override { return "Scan: " + access_method->schema().name; };
};

struct LogicalSelection : LogicalOperator {
    LogicalOperatorRef input;
    LogicalExprRef expr;

    LogicalSelection(LogicalOperatorRef input, LogicalExprRef expr) : input(std::move(input)), expr(std::move(expr)){};

    Table schema() const override { return input->schema(); };

    std::vector<LogicalOperatorRef> children() override;

    std::string toString() const override { return "Selection: " + expr->toString(); };
};

// TODO: projection currently only works with column logical expressions,
// but it should work with other logical expressions. For example, it should be possible
// to make a query like: SELECT foo * 100 FROM ...
struct LogicalProjection : LogicalOperator {
    LogicalOperatorRef input;
    std::vector<ColumnLogicalExpr> proj_cols;

    LogicalProjection(LogicalOperatorRef input, std::vector<ColumnLogicalExpr> proj_cols)
        : input(std::move(input)), proj_cols(std::move(proj_cols)){};

    std::vector<LogicalOperatorRef> children() override;

    Table schema() const override;

    std::string toString() const override;
};
} // namespace somedb