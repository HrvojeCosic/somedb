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
using LogicalOperatorrRef = std::unique_ptr<LogicalOperator>;

struct LogicalOperator {
    virtual ~LogicalOperator() = default;

    // Gets the schema of the logical plan operator
    virtual Table schema() const = 0;

    // Gets the children of the
    virtual std::vector<LogicalOperatorrRef> children() const = 0;

    // Returns the plan of the operator as a formatted string
    virtual std::string toString() const = 0;

    // Prints the logical plan from the top (starting from OP) in a readable format (helps in debugging)
    static void pretty(LogicalOperatorrRef op, int indent);
};

struct LogicalScan : LogicalOperator {
    AccessMethodRef access_method;

    LogicalScan(AccessMethodRef access_method) : access_method(std::move(access_method)){};

    Table schema() const override { return access_method->schema(); };

    std::vector<LogicalOperatorrRef> children() const override { return std::vector<LogicalOperatorrRef>(); };

    std::string toString() const override { return "Scan: " + access_method->schema().name; };
};

struct LogicalSelection : LogicalOperator {
    LogicalOperatorrRef input;
    std::unique_ptr<LogicalExpr> expr;

    LogicalSelection(LogicalOperatorrRef input, std::unique_ptr<LogicalExpr> expr)
        : input(std::move(input)), expr(std::move(expr)){};

    Table schema() const override { return input->schema(); };

    std::vector<LogicalOperatorrRef> children() const override { return input->children(); };

    std::string toString() const override { return "Selection: " + expr->toString(); };
};
} // namespace somedb