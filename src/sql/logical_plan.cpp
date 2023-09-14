#include <algorithm>
#include <iterator>

#include "../../include/sql/logical_plan.hpp"
#include "../../include/sql/schema.hpp"

namespace somedb {

void LogicalOperator::pretty(LogicalOperatorRef op, int indent) {
    std::cout << "\n";
    for (int i = 0; i < indent; i++) {
        std::cout << "\t";
    }
    std::cout << op->toString();
    for (int j = 0; j < (int)op->children().size(); j++) {
        pretty(std::move(op->children().at(j)), indent + 1);
    }
};

std::vector<LogicalOperatorRef> LogicalOperator::children() {
    auto ch = std::vector<LogicalOperatorRef>();
    ch.emplace_back(std::move(input));
    return ch;
};

Table LogicalProjection::schema() const {
    std::vector<Column> new_cols;
    for (const auto &col : proj_cols) {
        new_cols.emplace_back(Column(col.name, col.return_type));
    }
    return Table(input->schema().name, new_cols);
};

std::string LogicalProjection::toString() const {
    std::string res = "Projection: ";
    for (const auto &col : proj_cols)
        res += col.toString() + "\t";

    return res;
};
} // namespace somedb