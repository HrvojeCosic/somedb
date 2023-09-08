#pragma once

#include "../utils/shared.h"
#include "./schema.hpp"

#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

namespace somedb {

struct SqlExpr;
struct LogicalExpr;
using SqlExprRef = std::unique_ptr<SqlExpr>;
using LogicalExprRef = std::unique_ptr<LogicalExpr>;

struct ExprValue;

//-----------------------------------------------------------------------------------------------------------------

enum TypeId {  BOOLEAN, INTEGER, DECIMAL, VARCHAR, TIMESTAMP, INVALID };
enum CmpState  { SQL_TRUE, SQL_FALSE, SQL_NULL };

struct ExprType {
    TypeId type_id;

    ExprType(TypeId type_id = INVALID) : type_id(type_id){};

    virtual CmpState equals(ExprValue& l, ExprValue& r);
    virtual CmpState notEquals(ExprValue& l, ExprValue& r);
    virtual CmpState greaterThan(ExprValue& l, ExprValue& r);
    virtual CmpState lessThan(ExprValue& l, ExprValue& r);
};

struct ExprValue {
    ExprType type;

    ExprValue(ExprType type, u8 b);
    ExprValue(ExprType type, int32_t i);
    ExprValue(ExprType type, double d);
    ExprValue(ExprType type, char *val, u16 vc);
    ExprValue(ExprType type, int64_t ts);

    union {
        u8 boolean;
        int32_t integer;
        double decimal;
        char *varchar;
        int64_t timestamp;
    } value;

    u16 length; // for str type
};

//-----------------------------------------------------------------------------------------------------------------
/*
 * Structs inheriting from LogicalExpr are the structs used *at the query planning level*. Their main purpose is
 * to allow the planner to evaluate these different types of expressions at runtime against data from the relevant
 * schema
 */
struct LogicalExpr {
    LogicalExpr(std::vector<LogicalExprRef> children) : children(std::move(children)){};
    virtual ~LogicalExpr() = default;
    virtual ExprValue evaluate(Row &row, Table &table) = 0;
    virtual std::string toString() const = 0;

    std::vector<LogicalExprRef> children;
    ExprType return_type;
};

//-----------------------------------------------------------------------------------------------------------------

/*
 * Structs inheriting from SqlExpr are the structs used for determining types of expressions *at the parser level*.
 * They're used by the parser in order for it to decide how to parse an expression and to make sure the expression is
 * following the grammar
 */
struct SqlExpr {
    virtual ~SqlExpr() = default;
    virtual std::string toString() const = 0;
};

struct SqlBinaryExpr : SqlExpr {
    std::string op;
    SqlExprRef left;
    SqlExprRef right;

    SqlBinaryExpr(SqlExprRef left, std::string op, SqlExprRef right);

    std::string toString() const override;
};

struct SqlIdentifier : SqlExpr {
    SqlIdentifier(std::string identifier);

    std::string toString() const override;

  private:
    std::string identifier;
};

struct SqlSelect : SqlExpr {
    const std::string table_name;
    std::vector<SqlExprRef> projection;
    std::optional<SqlExprRef> selection;

    SqlSelect(const std::string table_name, std::vector<SqlExprRef> &projection);

    std::string toString() const override;
};

//-----------------------------------------------------------------------------------------------------------------
} // namespace somedb