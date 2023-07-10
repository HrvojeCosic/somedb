#pragma once

#include <cstdint>
#include <utility>
#include <vector>

#define INDEX_PAGE_TEMPLATE template <typename KeyType, typename ValueType, typename KeyComparator>

#include "./shared.h"

namespace somedb {

enum class IndexPageType { LEAF, INTERNAL };

struct BPlusTreePage {
    uint16_t level;
    uint32_t free_slots; // number of free slots available in a node
    IndexPageType page_type;
    uint32_t size;
    uint32_t max_size;
};

INDEX_PAGE_TEMPLATE struct BPlusTreeInternalPage : BPlusTreePage {
    std::vector<std::pair<KeyType, BPlusTreePage *>>
        data; // n keys and n+1 child pointers pairs
};

INDEX_PAGE_TEMPLATE struct BPlusTreeLeafPage : BPlusTreePage {
    BPlusTreeLeafPage *previous;
    BPlusTreeLeafPage *next;
    std::vector<KeyType> keys; // ordered keys mapping to corresponding values of ValueType
    std::vector<ValueType> values;
};

} // namespace somedb
