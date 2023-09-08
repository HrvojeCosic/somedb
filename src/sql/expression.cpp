#include "../../include/sql/expression.hpp"

namespace somedb {

//-----------------------------------------------------------------------------------------------------------------
SqlIdentifier::SqlIdentifier(std::string identifier) : identifier(identifier){};

SqlBinaryExpr::SqlBinaryExpr(SqlExprRef left, std::string op, SqlExprRef right)
    : op(op), left(std::move(left)), right(std::move(right)){};

SqlSelect::SqlSelect(const std::string table_name, std::vector<SqlExprRef> &projection)
    : table_name(table_name), projection(std::move(projection)){};

//-----------------------------------------------------------------------------------------------------------------

std::string SqlBinaryExpr::toString() const { return left->toString() + op + right->toString(); };

std::string SqlIdentifier::toString() const { return identifier; };

std::string SqlSelect::toString() const {
    std::string str = "SELECT ";
    for (uint i = 0; i < projection.size(); i++)
        str += projection.at(i)->toString() + ", ";
    str += selection ? "WHERE " + selection->get()->toString() : " ";
    return str;
};

//-----------------------------------------------------------------------------------------------------------------
} // namespace somedb