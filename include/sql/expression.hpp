#pragma once

#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

namespace somedb {

struct SqlExpr {
    virtual ~SqlExpr() = default;
    virtual std::string toString() = 0;
};

struct SqlBinaryExpr : SqlExpr {
    std::string op;
    std::unique_ptr<SqlExpr> left;
    std::unique_ptr<SqlExpr> right;

    SqlBinaryExpr(std::unique_ptr<SqlExpr> left, std::string op, std::unique_ptr<SqlExpr> right)
        : op(op), left(std::move(left)), right(std::move(right)){};

    inline std::string toString() override { return left->toString() + op + right->toString(); };
};

struct SqlIdentifier : SqlExpr {
    SqlIdentifier(std::string identifier) : identifier(identifier){};

    inline std::string toString() override { return identifier; };

  private:
    std::string identifier;
};

struct SqlSelect : SqlExpr {
    const std::string table_name;
    std::vector<std::unique_ptr<SqlExpr>> projection;
    std::optional<std::unique_ptr<SqlExpr>> selection;

    SqlSelect(const std::string table_name, std::vector<std::unique_ptr<SqlExpr>> &projection)
        : table_name(table_name), projection(std::move(projection)){};

    inline std::string toString() override {
        std::string str = "SELECT ";
        for (uint i = 0; i < projection.size(); i++)
            str += projection.at(i)->toString() + ", ";
        str += selection ? "WHERE " + selection->get()->toString() : " ";
        return str;
    };
};
} // namespace somedb