#include <stdexcept>
#include <string>

#include "../../include/sql/logical_expression.hpp"

namespace somedb {

PrimitiveValue BinaryLogicalExpr::evaluate(Row &row, Table &table) {
    const PrimitiveValue lhs = children.at(0)->evaluate(row, table);
    const PrimitiveValue rhs = children.at(1)->evaluate(row, table);

    PrimitiveValue res;
    switch (op) {
    case ADD:
        res = lhs.type->add(lhs, rhs);
        break;
    case SUBTRACT:
        res = lhs.type->subtract(lhs, rhs);
        break;
    case MULTIPLY:
        res = lhs.type->multiply(lhs, rhs);
        break;
    case DIVIDE:
        res = lhs.type->divide(lhs, rhs);
        break;
    };

    return res;
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

PrimitiveValue ColumnLogicalExpr::evaluate(Row &row, Table &table) {
    auto same_name = [this](Column col) { return col.name == name; };
    auto schema_col = std::find_if(table.columns.begin(), table.columns.end(), same_name);
    if (schema_col == table.columns.end())
        throw std::runtime_error("Column " + name + " not found in the schema");

    return row.getValue(*schema_col, table);
};
} // namespace somedb