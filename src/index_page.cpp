#include "../include/index_page.hpp"
#include "../include/disk_manager.h"
#include "../include/heapfile.h"
#include "../include/serialize.h"
#include <algorithm>
#include <bits/ranges_util.h>
#include <cassert>
#include <cstring>
#include <iostream>
#include <memory>
#include <stack>
#include <string>
#include <unistd.h>

namespace somedb {

BTreePage::~BTreePage() {
    for (auto &key : keys) {
        delete[] key.data;
    }
}

BTreePage::BTreePage(bool is_leaf) : is_leaf(is_leaf) {
    if (is_leaf)
        values = std::vector<RID>();
    else
        values = std::vector<u32>();
}

u8 *BTreePage::serialize() const {
    u8 *data = new u8[PAGE_SIZE];
    u16 available_space_start = INDEX_PAGE_HEADER_SIZE;
    u16 available_space_end = PAGE_SIZE;

    // populate data with kv pairs and their pointers in slotted page format
    int i = keys.empty() ? 0 : (keys.size() - 1);
    while (i >= 0) {
        auto key = keys.at(i);
        u8 val_size = 0;
        u8 val_buf[RID_SIZE];
        if (is_leaf) {
            RID val = LEAF_RECORDS(values).at(i);
            encode_uint32(val.pid, val_buf);
            encode_uint32(val.slot_num, val_buf + sizeof(u32));
            val_size += RID_SIZE;
        } else {
            auto children = INTERNAL_CHILDREN(values);
            encode_uint32(children.at(i), val_buf);
            val_size += sizeof(u32);
        }
        u16 kv_size = val_size + key.length + sizeof(key.length);
        u16 kv_start = available_space_end - kv_size;
        memcpy(data + kv_start, &key.length, 1);
        memcpy(data + kv_start + 1, key.data, key.length);
        memcpy(data + kv_start + 1 + key.length, val_buf, val_size);

        u16 kv_ptr_start = available_space_start;
        encode_uint16(kv_start, data + kv_ptr_start);
        encode_uint16(kv_size, data + kv_ptr_start + sizeof(u16));

        available_space_end -= kv_size;
        available_space_start += TREE_KV_PTR_SIZE;
        i--;
    }

    // encode the only value without its corresponding key as a rightmost pointer
    if (!is_leaf)
        encode_uint32(INTERNAL_CHILDREN(values).back(), data + RIGHTMOST_PID_OFFSET);

    encode_uint16(available_space_start, data + AVAILABLE_SPACE_START_OFFSET);
    encode_uint16(available_space_end, data + AVAILABLE_SPACE_END_OFFSET);
    encode_uint32(previous, data + PREVIOUS_PID_OFFSET);
    encode_uint32(next, data + NEXT_PID_OFFSET);
    encode_uint16(rightmost_ptr, data + RIGHTMOST_PID_OFFSET);
    encode_uint16(level, data + TREE_LEVEL_OFFSET);
    memcpy(data + TREE_FLAGS_OFFSET, &flags, TREE_FLAGS_SIZE);
    memcpy(data + IS_LEAF_OFFSET, &is_leaf, IS_LEAF_SIZE);

    return data;
}

BTreePage::BTreePage(u8 data[PAGE_SIZE]) {
    previous = decode_uint32(data + PREVIOUS_PID_OFFSET);
    next = decode_uint32(data + NEXT_PID_OFFSET);
    rightmost_ptr = decode_uint32(data + RIGHTMOST_PID_OFFSET);
    level = decode_uint16(data + TREE_LEVEL_OFFSET);
    memcpy(&flags, data + TREE_FLAGS_OFFSET, TREE_FLAGS_SIZE);
    memcpy(&is_leaf, data + IS_LEAF_OFFSET, IS_LEAF_SIZE);

    if (is_leaf)
        values = std::vector<RID>();
    else
        values = std::vector<u32>();

    u16 kv_pairs_cursor = decode_uint16(data + AVAILABLE_SPACE_END_OFFSET);
    while (kv_pairs_cursor < PAGE_SIZE) {
        u8 curr_key_len = *(data + kv_pairs_cursor);

        // skip the empty kv slots
        if (curr_key_len == 0) {
            is_leaf ? kv_pairs_cursor += RID_SIZE : TREE_KV_PTR_SIZE;
            continue;
        }

        BTreeKey key = {.data = 0, .length = curr_key_len};
        key.data = new u8[key.length];
        memcpy(key.data, data + kv_pairs_cursor + 1, key.length);

        kv_pairs_cursor += sizeof(key.length) + key.length;
        keys.emplace_back(key);

        if (is_leaf) {
            RID rid;
            rid.pid = decode_uint32(data + kv_pairs_cursor);
            rid.slot_num = decode_uint32(data + kv_pairs_cursor + sizeof(rid.pid));
            LEAF_RECORDS(values).emplace_back(rid);
            kv_pairs_cursor += RID_SIZE;
        } else {
            INTERNAL_CHILDREN(values).emplace_back(decode_uint32(data + kv_pairs_cursor));
            kv_pairs_cursor += TREE_KV_PTR_SIZE;
        }
    }
}

TREE_NODE_FUNC_TYPE void BTreePage::insertIntoNode(const BTreeKey &key, VAL_T val) {
    std::vector<VAL_T> node_values;
    if constexpr (std::is_same_v<VAL_T, RID>)
        node_values = LEAF_RECORDS(values);
    else
        node_values = INTERNAL_CHILDREN(values);

    uint16_t size = keys.size();
    keys.resize(size + 1);
    node_values.resize(size + 1);
    auto first_key = keys.at(0);
    auto last_key = keys.at(size == 0 ? size : size - 1);

    // New key is larger (or equal) than all current ones
    if (cmpKeys(key.data, last_key.data, key.length, last_key.length) >= 0) {
        keys.at(keys.size() - 1) = key;
        node_values.at(node_values.size() - 1) = val;
    }
    // New key is smaller (or equal) than all current ones
    else if (cmpKeys(key.data, first_key.data, key.length, first_key.length) <= 0) {
        std::vector<BTreeKey> new_keys;
        new_keys.reserve(size + 1);
        new_keys.push_back(key);
        new_keys.insert(new_keys.end(), keys.begin(), keys.end() - 1);
        keys = new_keys;

        std::vector<VAL_T> new_vals;
        new_vals.reserve(size + 1);
        new_vals.push_back(val);
        new_vals.insert(new_vals.end(), node_values.begin(), node_values.end() - 1);
        node_values = new_vals;
    }
    // New key is in the middle.
    // Insert it at appropriate index and shift all following elements to right
    else {
        for (uint16_t i = 0; i < size; i++) {
            auto curr_key = keys.at(i);
            auto next_key = keys.at(i + 1);
            if (cmpKeys(key.data, curr_key.data, key.length, curr_key.length) > 0 &&
                cmpKeys(key.data, next_key.data, key.length, next_key.length) <= 0) {
                // shift all elements to right (for both keys and values)
                BTreeKey prev_key = keys.at(i + 1);
                VAL_T prev_value = node_values.at(i + 1);
                keys.at(i + 1) = key;
                node_values.at(i + 1) = val;
                for (int j = i; j < size - 1; j++) {
                    BTreeKey temp_key = keys.at(j + 2);
                    VAL_T temp_val = node_values.at(j + 2);
                    keys.at(j + 2) = prev_key;
                    node_values.at(j + 2) = prev_value;
                    prev_key = temp_key;
                    prev_value = temp_val;
                }
                break;
            }
        }
    }

    values = node_values;
}

template void BTreePage::insertIntoNode<RID>(const BTreeKey &, RID);
template void BTreePage::insertIntoNode<u32>(const BTreeKey &, u32);

} // namespace somedb
