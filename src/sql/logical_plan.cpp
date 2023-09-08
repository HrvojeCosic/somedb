#include "../../include/sql/logical_plan.hpp"

namespace somedb {

//-----------------------------------------------------------------------------------------------------------------
LogicalScan::LogicalScan(AccessMethodRef access_method) : access_method(std::move(access_method)){};

LogicalSelection::LogicalSelection(LogicalOperatorrRef input, std::unique_ptr<LogicalExpr> expr)
    : input(std::move(input)), expr(std::move(expr)){};

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

//-----------------------------------------------------------------------------------------------------------------
Table LogicalScan::schema() { return access_method->schema(); };

Table LogicalSelection::schema() { return input->schema(); };

//-----------------------------------------------------------------------------------------------------------------
std::vector<LogicalOperatorrRef> LogicalScan::children() { return std::vector<LogicalOperatorrRef>(); };

std::vector<LogicalOperatorrRef> LogicalSelection::children() { return input->children(); };

//-----------------------------------------------------------------------------------------------------------------
std::string LogicalScan::toString() { return "Scan: " + access_method->schema().name; };

std::string LogicalSelection::toString() { return "Selection: " + expr->toString(); };

//-----------------------------------------------------------------------------------------------------------------
} // namespace somedb