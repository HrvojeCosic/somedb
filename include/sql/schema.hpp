#pragma once

#include <string>
#include <vector>

namespace somedb {

// Logical representation of a tuple
using Row = std::vector<char>;

// A table column schema
struct Column {
    std::string name;
    std::string data_type;
};

// A table schema
struct Table {
    std::string name;
    std::vector<Column> columns;
};
} // namespace somedb