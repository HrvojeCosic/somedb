#include "../../include/sql/logical_expression.hpp"

namespace somedb {

PrimitiveValue BinaryLogicalExpr::evaluate(Row &row, Table &table) {
    const PrimitiveValue lhs = children.at(0)->evaluate(row, table);
    const PrimitiveValue rhs = children.at(1)->evaluate(row, table);

    switch (op) {
    case ADD:
        return lhs.type->add(lhs, rhs);
    case SUBTRACT:
        return lhs.type->subtract(lhs, rhs);
    case MULTIPLY:
        return lhs.type->multiply(lhs, rhs);
    case DIVIDE:
        return lhs.type->divide(lhs, rhs);
    };
};

std::string BinaryLogicalExpr::toString() const {
    auto lhs = children.at(0);
    auto rhs = children.at(1);
    std::string op_str;

    switch (op) {
    case ADD:
        op_str = "+";
        break;
    case SUBTRACT:
        op_str = "-";
        break;
    case MULTIPLY:
        op_str = "*";
        break;
    case DIVIDE:
        op_str = "/";
        break;
    }

    return lhs->toString() + op_str + rhs->toString();
};
} // namespace somedb