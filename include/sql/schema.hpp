#pragma once

#include "./primitive.hpp"

#include <string>
#include <vector>

namespace somedb {

// Logical representation of a column
struct Column {
    std::string name;
    PrimitiveTypeRef data_type;

    Column(std::string name, PrimitiveTypeRef data_type) : name(name), data_type(data_type){};

    bool operator==(const Column &other) {
        return other.name == name && other.data_type->toString() == data_type->toString();
    }
};

// Logical representation of a table (schema)
struct Table {
    std::string name;
    std::vector<Column> columns;

    Table(std::string name, std::vector<Column> columns) : name(name), columns(columns){};

    bool operator==(const Table &other) {
        if (other.name != name || other.columns.size() != columns.size())
            return false;

        for (uint i = 0; i < columns.size(); i++) {
            if (columns[i].name != other.columns[i].name ||
                columns[i].data_type->toString() != other.columns[i].data_type->toString())
                return false;
        }

        return true;
    }
};

// Logical representation of a tuple
struct Row {
    u8 *data;

    // Gets the value under a specified column
    PrimitiveValue getValue(Column &col, Table &schema);
};
} // namespace somedb