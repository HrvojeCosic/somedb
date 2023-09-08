#pragma once

#include "./access_method.hpp"
#include "./expression.hpp"
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
    virtual Table schema() = 0;

    // Gets the children of the
    virtual std::vector<LogicalOperatorrRef> children() = 0;

    // Returns the plan of the operator as a formatted string
    virtual std::string toString() = 0;

    // Prints the logical plan from the top (starting from OP) in a readable format (helps in debugging)
    static void pretty(LogicalOperatorrRef op, int indent);
};

struct LogicalScan : LogicalOperator {
    AccessMethodRef access_method;
    LogicalScan(AccessMethodRef access_method);

    Table schema() override;
    std::vector<LogicalOperatorrRef> children() override;
    std::string toString() override;
};

struct LogicalSelection : LogicalOperator {
    LogicalOperatorrRef input;
    std::unique_ptr<LogicalExpr> expr;
    LogicalSelection(LogicalOperatorrRef input, std::unique_ptr<LogicalExpr> expr);

    Table schema() override;
    std::vector<LogicalOperatorrRef> children() override;
    std::string toString() override;
};
} // namespace somedb