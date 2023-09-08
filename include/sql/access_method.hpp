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
    virtual ~AccessMethod() = default;
    virtual Table schema() const = 0;
    virtual std::vector<std::string> scan() const = 0; // TODO: actual rows, not strings
};

struct HeapfileAccess : AccessMethod {
    std::vector<std::string> scan() const { throw std::runtime_error("TODO: Heapfile scan implementation"); };
};

struct BTreeIndexAccess : AccessMethod {
    std::vector<std::string> scan() const { throw std::runtime_error("TODO: B+tree index scan implementation"); };
};
} // namespace somedb