#pragma once

#include "./primitive.hpp"
#include "./schema.hpp"
#include "./sql_expression.hpp"

namespace somedb {

struct LogicalExpr;
using LogicalExprRef = std::shared_ptr<LogicalExpr>;

/*
 * Structs inheriting from LogicalExpr are the structs used *at the query planning level*. Their main purpose is
 * to allow the planner to evaluate these different types of expressions at runtime against data from the relevant
 * schema
 */
struct LogicalExpr {
    LogicalExpr(std::vector<LogicalExprRef> children) : children(std::move(children)){};

    virtual ~LogicalExpr() = default;
    virtual PrimitiveValue evaluate(Row &row, Table &table) = 0;
    virtual std::string toString() const = 0;

    std::vector<LogicalExprRef> children;
    PrimitiveTypeRef return_type;
};

struct ConstantLogicalExpr : LogicalExpr {
    PrimitiveValue value;

    ConstantLogicalExpr(PrimitiveValue val) : LogicalExpr({}), value(std::move(val)){};

    PrimitiveValue evaluate(Row &, Table &) override { return value; };

    inline std::string toString() const override { return value.toString(); };
};

enum BinaryOpType { ADD, SUBTRACT, DIVIDE, MULTIPLY };

struct BinaryLogicalExpr : LogicalExpr {
    BinaryOpType op;

    BinaryLogicalExpr(LogicalExprRef left, LogicalExprRef right, BinaryOpType op)
        : LogicalExpr({std::move(left), std::move(right)}), op(op){};

    PrimitiveValue evaluate(Row &, Table &) override;
    inline std::string toString() const override;
};
} // namespace somedb