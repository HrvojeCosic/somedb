#pragma once

#include "./schema.hpp"

#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace somedb {

struct AccessMethod;
using AccessMethodRef = std::unique_ptr<AccessMethod>;

struct AccessMethod {
    std::string path;

    AccessMethod(std::string path) : path(path){};
    virtual ~AccessMethod() = default;

    virtual Table schema() const = 0;
    virtual std::vector<Row> scan() const = 0;
};

struct HeapfileAccess : AccessMethod {
    HeapfileAccess(std::string table_name) : AccessMethod(table_name){};

    std::vector<Row> scan() const override { throw std::runtime_error("TODO: Heapfile scan implementation"); };

    Table schema() const override;
};

struct BTreeIndexAccess : AccessMethod {
    std::vector<Row> scan() const { throw std::runtime_error("TODO: B+tree index scan implementation"); };
};
} // namespace somedb