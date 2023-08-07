#include "../include/index.hpp"
#include "../include/disk_manager.h"
#include "../include/heapfile.h"
#include "../include/serialize.h"
#include <algorithm>
#include <bits/ranges_util.h>
#include <cassert>
#include <cstring>
#include <iostream>
#include <stack>
#include <unistd.h>

namespace somedb {

u8 *BTree::serialize() const {
    u8 *data = new u8[PAGE_SIZE];
    u16 cursor = 0;

    encode_uint32(this->magic_num, data);
    cursor += sizeof(u32);
    encode_uint16(this->node_count, data);
    cursor += sizeof(u16);
    memcpy(data + cursor, &this->max_size, sizeof(u8));
    cursor += sizeof(u8);

    return data;
}

void BTree::deserialize(u8 data[PAGE_SIZE]) {
    u16 cursor = 0;

    this->magic_num = decode_uint32(data + cursor);
    cursor += sizeof(u32);
    this->node_count = decode_uint16(data + cursor);
    cursor += sizeof(u16);
    memcpy(&this->max_size, data + cursor, sizeof(u8));
    cursor += sizeof(u8);
}

TREE_NODE_TYPE u8 *BTreePage<VAL_T>::serialize() const {
    u8 *data = new u8[PAGE_SIZE];
    u16 cursor = 0;
    u16 available_space_start = INDEX_PAGE_HEADER_SIZE;
    u16 available_space_end = PAGE_SIZE;

    // populate data with kv pairs and their pointers in slotted page format
    for (u16 i = 0; i < this->keys.size(); i++) {
        auto key = this->keys.at(i);
        u8 val_size;
        u8 val_buf[RID_SIZE];
        if constexpr (std::same_as<VAL_T, RID>) {
            RID val = LEAF_RECORDS(this->records).at(i);
            encode_uint32(val.pid, val_buf);
            encode_uint32(val.slot_num, val_buf + sizeof(u32));
            val_size = RID_SIZE;
        } else {
            auto children = INTERNAL_CHILDREN(this->children);
            encode_uint32(children.at(i), val_buf);
            val_size = sizeof(u32);
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
        available_space_start += sizeof(u16) * 2;
    }

    encode_uint16(available_space_start, data + cursor);
    cursor += sizeof(u16);
    encode_uint16(available_space_end, data + cursor);
    cursor += sizeof(u16);
    encode_uint32(this->previous_off, data + cursor);
    cursor += sizeof(u32);
    encode_uint32(this->next_off, data + cursor);
    cursor += sizeof(u32);
    encode_uint16(this->rightmost_ptr_off, data + cursor);
    cursor += sizeof(u16);
    encode_uint16(this->level, data + cursor);
    cursor += sizeof(u16);
    memcpy(data + cursor, &this->flags, sizeof(u8));
    cursor += sizeof(u8);

    return data;
}

TREE_NODE_TYPE void BTreePage<VAL_T>::deserialize(u8 data[PAGE_SIZE]) {
    u16 cursor = sizeof(u16) * 2; // skip available space offsets

    this->previous_off = decode_uint32(data + cursor);
    cursor += sizeof(u32);
    this->next_off = decode_uint32(data + cursor);
    cursor += sizeof(u32);
    this->rightmost_ptr_off = decode_uint16(data + cursor);
    this->is_leaf = this->rightmost_ptr_off == 0;
    cursor += sizeof(u16);
    this->level = decode_uint16(data + cursor);
    cursor += sizeof(u16);
    memcpy(&this->flags, data + cursor, sizeof(u8));
    cursor += sizeof(u8);

    u16 kv_pairs_cursor = decode_uint16(data + sizeof(u16));
    while (kv_pairs_cursor < PAGE_SIZE) {
        BTreeKey key = {.data = 0, .length = 0};
        key.length = *(data + kv_pairs_cursor);
        key.data = new u8[key.length];
        memcpy(key.data, data + kv_pairs_cursor + 1, key.length);
        this->keys.emplace_back(key);

        kv_pairs_cursor += sizeof(key.length) + key.length;
        if constexpr (std::same_as<VAL_T, RID>) {
            RID rid;
            rid.pid = decode_uint32(data + kv_pairs_cursor);
            rid.slot_num = decode_uint32(data + kv_pairs_cursor + sizeof(u32));
            LEAF_RECORDS(this->records).emplace_back(rid);
            kv_pairs_cursor += RID_SIZE;
        } else {
            INTERNAL_CHILDREN(this->children).emplace_back(decode_uint32(data + kv_pairs_cursor));
            kv_pairs_cursor += sizeof(u32);
        }
    }
}
// bool BTreePage::getValues(const BTreeKey &key, std::vector<RID> &result) { return false; }
//
// bool BTreePage::insert(const BTreeKey &key, const RID &val) {
//     auto breadcrumbs = std::stack<BREADCRUMB_TYPE>();
//     BTreePage *leaf = findLeaf(key, breadcrumbs);
//
//     // return early if duplicate key
//     for (BTreeKey leaf_key : leaf->keys)
//         if (cmpKeys(leaf_key.data, key.data, leaf_key.length, key.length) == 0)
//             return false;
//
//     insertIntoNode<RID>(leaf, key, val);
//
//     if (leaf->max_size < leaf->keys.size()) {
//         if (leaf == root)
//             splitRootNode(leaf);
//         else
//             throw std::runtime_error("NON-ROOT SPLITS NOT YET IMPLEMENTED"); // TODO
//     }
//
//     return true;
// }
//
// void BTreePage::splitRootNode(BTreePage *node) {
//     auto mid_key = node->keys.at(node->keys.size() / 2);
//
//     auto new_node = new BTreePage(true, node->max_size);
//     auto new_root = new BTreePage(false, node->max_size);
//
//     // redistribute keys/values between old and new node
//     new_node->keys = vec_second_half<BTreeKey>(node->keys);
//     node->keys = vec_first_half(node->keys);
//     auto full = LEAF_RECORDS(node->records);
//     new_node->records = vec_second_half<RID>(full);
//     node->records = vec_first_half<RID>(full);
//
//     new_node->previous = node;
//     node->next = new_node;
//
//     // connect nodes to new root
//     new_root->keys.insert(new_root->keys.begin(), mid_key);
//     INTERNAL_CHILDREN(new_root->children).insert(INTERNAL_CHILDREN(new_root->children).begin(), node);
//     INTERNAL_CHILDREN(new_root->children).insert(INTERNAL_CHILDREN(new_root->children).begin() + 1, new_node);
//     this->root = new_root;
// }
//
// BTreePage *BTreePage::findLeaf(const BTreeKey &key, std::stack<BREADCRUMB_TYPE> &breadcrumbs) {
//     BTreePage *temp = this->root;
//
//     while (!temp->is_leaf) {
//         auto first_key = temp->keys[0];
//         if (cmpKeys(first_key.data, key.data, first_key.length, key.length) < 0) {
//             breadcrumbs.push(BREADCRUMB_TYPE(temp, 0));
//             temp = std::get<internal_pointers>(temp->children).at(0);
//             continue;
//         }
//         for (u16 i = 1; i < temp->keys.size(); i++) {
//             auto prev_key = temp->keys[i - 1];
//             auto curr_key = temp->keys[i];
//             if (cmpKeys(prev_key.data, key.data, prev_key.length, key.length) > 0 &&
//                 cmpKeys(curr_key.data, key.data, curr_key.length, key.length) <= 0) {
//                 breadcrumbs.push(BREADCRUMB_TYPE(temp, i));
//                 temp = std::get<internal_pointers>(temp->children).at(i);
//                 continue;
//             }
//         }
//
//         breadcrumbs.push(BREADCRUMB_TYPE(temp, -1)); // set it to the rightmost child pointer
//         temp = temp->rightmost_ptr;
//     }
//
//     breadcrumbs.push(BREADCRUMB_TYPE(temp, 1));
//     return temp;
// }
//
// TREE_NODE_FUNC_TYPE
// void BTreePage::insertIntoNode(BTreePage *node, const BTreeKey &key, VAL_T val) {
//     std::vector<VAL_T> node_values;
//     if constexpr (std::is_same_v<VAL_T, RID>)
//         node_values = LEAF_RECORDS(node->records);
//     else
//         node_values = INTERNAL_CHILDREN(node->children);
//
//     uint16_t size = node->keys.size();
//     node->keys.resize(size + 1);
//     node_values.resize(size + 1);
//     auto first_key = node->keys.at(0);
//     auto last_key = node->keys.at(size == 0 ? size : size - 1);
//
//     // New key is larger (or equal) than all current ones
//     if (cmpKeys(key.data, last_key.data, key.length, last_key.length) >= 0) {
//         node->keys.at(node->keys.size() - 1) = key;
//         node_values.at(node_values.size() - 1) = val;
//     }
//     // New key is smaller (or equal) than all current ones
//     else if (cmpKeys(key.data, first_key.data, key.length, first_key.length) <= 0) {
//         std::vector<BTreeKey> new_keys;
//         new_keys.reserve(size);
//         new_keys.push_back(key);
//         new_keys.insert(new_keys.end(), node->keys.begin(), node->keys.end() - 1);
//         node->keys = new_keys;
//
//         std::vector<VAL_T> new_vals;
//         new_vals.reserve(size + 1);
//         new_vals.push_back(val);
//         new_vals.insert(new_vals.end(), node_values.begin(), node_values.end() - 1);
//         node_values = new_vals;
//     }
//     // New key is in the middle.
//     // Insert it at appropriate index and shift all following elements to right
//     else {
//         for (uint16_t i = 0; i < size; i++) {
//             auto curr_key = node->keys.at(i);
//             auto next_key = node->keys.at(i + 1);
//             if (cmpKeys(key.data, curr_key.data, key.length, curr_key.length) > 0 &&
//                 cmpKeys(key.data, next_key.data, key.length, next_key.length) <= 0) {
//                 // shift all elements to right (for both keys and values)
//                 BTreeKey prev_key = node->keys.at(i + 1);
//                 VAL_T prev_value = node_values.at(i + 1);
//                 node->keys.at(i + 1) = key;
//                 node_values.at(i + 1) = val;
//                 for (int j = i; j < size - 1; j++) {
//                     BTreeKey temp_key = node->keys.at(j + 2);
//                     VAL_T temp_val = node_values.at(j + 2);
//                     node->keys.at(j + 2) = prev_key;
//                     node_values.at(j + 2) = prev_value;
//                     prev_key = temp_key;
//                     prev_value = temp_val;
//                 }
//                 break;
//             }
//         }
//     }
//
//     if constexpr (std::is_same_v<VAL_T, RID>)
//         node->records = node_values;
//     else
//         node->children = node_values;
//     return;
// }

template struct somedb::BTreePage<RID>;

} // namespace somedb
