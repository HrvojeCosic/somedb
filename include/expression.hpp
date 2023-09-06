#pragma once

#include <memory>
#include <string>

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
} // namespace somedb