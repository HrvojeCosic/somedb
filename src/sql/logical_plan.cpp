#include "../../include/sql/logical_plan.hpp"

namespace somedb {

void LogicalOperator::pretty(LogicalOperatorrRef op, int indent) {
    std::cout << "\n";
    for (int i = 0; i < indent; i++) {
        std::cout << "\t";
    }
    std::cout << op->toString();
    for (int j = 0; j < (int)op->children().size(); j++) {
        pretty(std::move(op->children().at(j)), indent + 1);
    }
};

} // namespace somedb