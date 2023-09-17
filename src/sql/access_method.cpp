#include "../../include/sql/access_method.hpp"
#include "../../include/disk/disk_manager.h"
#include "../../include/disk/heapfile.h"
#include "../../include/utils/serialize.h"

#include <cstring>
#include <stdexcept>

namespace somedb {

Table HeapfileAccess::schema() const {
    // Not an actual disk manager instance, just an adapter around the table name since read_page() accepts DiskManager
    DiskManager mgr{};
    mgr.table_name = const_cast<char *>(path.data());
    u8 *page_data = read_page(new_ptkey(mgr.table_name, TABLE_SCHEMA_PAGE));

    if (page_data == nullptr)
        throw std::runtime_error("Heapfile of provided name does not exist");

    // Decode table schema
    std::vector<Column> table_cols;
    u8 cols_num = *page_data;
    table_cols.reserve(cols_num);
    u8 cursor = START_COLUMNS_INFO;
    for (int i = 0; i < cols_num; i++) {
        u8 col_name_len = *(page_data + cursor);

        // Decode column name into a string
        char buf[col_name_len];
        memcpy(buf, page_data + cursor + sizeof(col_name_len), col_name_len);
        std::string col_name(buf, col_name_len);

        // Decode column type
        PrimitiveTypeRef col_type;
        ColumnType col_data_type = static_cast<ColumnType>(*(page_data + cursor + col_name_len + sizeof(col_name_len)));
        switch (col_data_type) {
        case BOOLEAN:
            col_type = std::make_unique<BooleanPrimitiveType>();
            break;
        case STRING:
            col_type = std::make_unique<VarcharPrimitiveType>();
            break;
        case INTEGER:
            col_type = std::make_unique<IntegerPrimitiveType>();
            break;
        case DECIMAL:
            col_type = std::make_unique<DecimalPrimitiveType>();
            break;
        }

        Column col(col_name, col_type);
        table_cols.emplace_back(col);

        cursor += SCHEMA_COLUMN_SIZE(col_name_len);
    }

    return Table(path, table_cols);
}
} // namespace somedb