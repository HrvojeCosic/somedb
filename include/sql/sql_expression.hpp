#pragma once

#include "../utils/shared.h"
#include "./primitive.hpp"
#include "./schema.hpp"

#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

namespace somedb {

struct SqlExpr;
using SqlExprRef = std::unique_ptr<SqlExpr>;

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

    SqlBinaryExpr(SqlExprRef left, std::string op, SqlExprRef right)
        : op(op), left(std::move(left)), right(std::move(right)){};

    inline std::string toString() const override { return left->toString() + op + right->toString(); };
};

struct SqlIdentifier : SqlExpr {
    SqlIdentifier(std::string identifier) : identifier(identifier){};

    inline std::string toString() const override { return identifier; };

  private:
    std::string identifier;
};

struct SqlSelect : SqlExpr {
    const std::string table_name;
    std::vector<SqlExprRef> projection;
    std::optional<SqlExprRef> selection;

    SqlSelect(const std::string table_name, std::vector<SqlExprRef> &projection)
        : table_name(table_name), projection(std::move(projection)){};

    std::string toString() const {
        std::string str = "SELECT ";
        for (uint i = 0; i < projection.size(); i++)
            str += projection.at(i)->toString() + ", ";
        str += selection ? "WHERE " + selection->get()->toString() : " ";
        return str;
    };
};

} // namespace somedb