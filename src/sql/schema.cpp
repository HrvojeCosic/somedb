#include "../../include/sql/schema.hpp"
#include <stdexcept>

namespace somedb {

PrimitiveValue Row::getValue(Column &col, Table &schema) {
    u32 curr_offset = 0;
    for (const auto &sch_col : schema.columns) {
        if (col.name == sch_col.name) {
            return col.data_type->deserialize(data + curr_offset);
        }
        curr_offset += col.data_type->getSize();
    }

    throw std::runtime_error("Specified column '" + col.name + "' does not exist in provided schema");
};
} // namespace somedb