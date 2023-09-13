#pragma once

#include "./primitive.hpp"
#include "./schema.hpp"
#include "./sql_expression.hpp"

#include <algorithm>

namespace somedb {

struct LogicalExpr;
using LogicalExprRef = std::shared_ptr<LogicalExpr>;

/*
 * Structs inheriting from LogicalExpr are the structs used *at the query planning level*. Their main purpose is
 * to allow the planner to evaluate these different types of expressions at runtime against data from the relevant
 * schema
 */
struct LogicalExpr {
    LogicalExpr(std::vector<LogicalExprRef> c, PrimitiveTypeRef t) : children(std::move(c)), return_type(t){};

    virtual ~LogicalExpr() = default;
    virtual PrimitiveValue evaluate(Row &row, Table &table) = 0;
    virtual std::string toString() const = 0;

    std::vector<LogicalExprRef> children;
    PrimitiveTypeRef return_type;
};

struct ConstantLogicalExpr : LogicalExpr {
    PrimitiveValue value;

    ConstantLogicalExpr(PrimitiveValue val) : LogicalExpr({}, val.type), value(std::move(val)){};

    PrimitiveValue evaluate(Row &, Table &) override { return value; };

    inline std::string toString() const override { return value.toString(); };
};

struct ColumnLogicalExpr : LogicalExpr {
    std::string name;

    ColumnLogicalExpr(std::string name, PrimitiveTypeRef ret_type) : LogicalExpr({}, ret_type), name(name){};

    PrimitiveValue evaluate(Row &row, Table &table) override;

    inline std::string toString() const override { return "Column: " + name; };
};

enum BinaryOpType { ADD, SUBTRACT, DIVIDE, MULTIPLY };

struct BinaryLogicalExpr : LogicalExpr {
    BinaryOpType op;

    BinaryLogicalExpr(LogicalExprRef l, LogicalExprRef r, BinaryOpType op)
        : LogicalExpr({std::move(l), std::move(r)}, l->return_type), op(op){};

    PrimitiveValue evaluate(Row &row, Table &) override;
    inline std::string toString() const override;
};
} // namespace somedb