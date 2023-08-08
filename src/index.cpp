#include "../include/index.hpp"
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

BTree::BTree(u8 data[PAGE_SIZE], BufferPoolManager *bpm)
    : magic_num(decode_uint32(data)), max_size(*(data + MAX_KEYSIZE_OFFSET)), bpm(bpm) {
    u16 cursor = sizeof(u32);
    this->node_count = decode_uint16(data + cursor);
    cursor += sizeof(u16);
    this->root_pid = decode_uint32(data + CURR_ROOT_PID_OFFSET);
}

u8 *BTreePage::serialize() const {
    u8 *data = new u8[PAGE_SIZE];
    u16 cursor = 0;
    u16 available_space_start = INDEX_PAGE_HEADER_SIZE;
    u16 available_space_end = PAGE_SIZE;

    // populate data with kv pairs and their pointers in slotted page format
    for (u16 i = 0; i < this->keys.size(); i++) {
        auto key = this->keys.at(i);
        u8 val_size;
        u8 val_buf[RID_SIZE];
        if (this->is_leaf) {
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
    encode_uint32(this->previous, data + cursor);
    cursor += sizeof(page_id_t);
    encode_uint32(this->next, data + cursor);
    cursor += sizeof(page_id_t);
    encode_uint16(this->rightmost_ptr, data + cursor);
    cursor += sizeof(page_id_t);
    encode_uint16(this->level, data + cursor);
    cursor += sizeof(u16);
    memcpy(data + cursor, &this->flags, sizeof(u8));
    cursor += sizeof(u8);

    return data;
}

BTreePage::BTreePage(u8 data[PAGE_SIZE]) : is_leaf(get_is_leaf(data)) {
    u16 cursor = sizeof(u16) * 2; // skip available space offsets

    this->previous = decode_uint32(data + cursor);
    cursor += sizeof(page_id_t);
    this->next = decode_uint32(data + cursor);
    cursor += sizeof(page_id_t);
    this->rightmost_ptr = decode_uint32(data + cursor);
    cursor += sizeof(page_id_t);
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
        if (key.length != 0) {
            this->keys.emplace_back(key);
        }

        kv_pairs_cursor += sizeof(key.length) + key.length;
        if (this->is_leaf) {
            RID rid;
            rid.pid = decode_uint32(data + kv_pairs_cursor);
            rid.slot_num = decode_uint32(data + kv_pairs_cursor + sizeof(u32));
            if (key.length != 0) {
                LEAF_RECORDS(this->records).emplace_back(rid);
            }
            kv_pairs_cursor += RID_SIZE;
        } else {
            if (key.length != 0) {
                INTERNAL_CHILDREN(this->children).emplace_back(decode_uint32(data + kv_pairs_cursor));
            }
            kv_pairs_cursor += sizeof(u32);
        }
    }
}

BTreePage::~BTreePage() {
    for (auto &key : keys) {
        delete[] key.data;
    }
}

bool BTree::insert(const BTreeKey &key, const RID &val) {
    auto breadcrumbs = std::stack<BREADCRUMB_TYPE>();

    page_id_t leaf_pid = 0;
    auto leaf = findLeaf(key, breadcrumbs, &leaf_pid);
    assert(leaf_pid != 0);

    // return early if duplicate key
    for (BTreeKey leaf_key : leaf->keys)
        if (BTreePage::cmpKeys(leaf_key.data, key.data, leaf_key.length, key.length) == 0)
            return false;

    leaf->insertIntoNode(key, val);

    if (this->max_size < leaf->keys.size()) {
        if (leaf->level == 0)
            splitRootNode();
        else
            throw std::runtime_error("NON-ROOT SPLITS NOT YET IMPLEMENTED"); // TODO
    }

    write_page(leaf_pid, bpm->disk_manager, leaf->serialize());
    return true;
}

void BTree::splitRootNode() {
    auto old_root_node = std::make_unique<BTreePage>(fetch_bpm_page(root_pid, bpm)->data);

    auto mid_key = old_root_node->keys.at(old_root_node->keys.size() / 2);

    auto new_node = std::make_unique<BTreePage>(false);
    auto new_root = std::make_unique<BTreePage>(true);
    page_id_t new_node_id = ++node_count;
    page_id_t new_root_id = ++node_count;

    // redistribute keys/values between old and new root node
    new_node->keys = BTreePage::vec_second_half<BTreeKey>(old_root_node->keys);
    old_root_node->keys = BTreePage::vec_first_half(old_root_node->keys);
    auto full = LEAF_RECORDS(old_root_node->records);
    new_node->records = BTreePage::vec_second_half<RID>(full);
    old_root_node->records = BTreePage::vec_first_half<RID>(full);

    new_node->previous = root_pid;
    old_root_node->next = new_node_id;

    // create new root and connect nodes to it
    new_root->keys.insert(new_root->keys.begin(), mid_key);
    INTERNAL_CHILDREN(new_root->children).insert(INTERNAL_CHILDREN(new_root->children).begin(), root_pid);
    INTERNAL_CHILDREN(new_root->children).insert(INTERNAL_CHILDREN(new_root->children).begin() + 1, new_node_id);
    root_pid = new_root_id;

    // persist all changes
    write_page(BTREE_METADATA_PAGE_ID, bpm->disk_manager, serialize());
    write_page(new_node_id, bpm->disk_manager, new_node->serialize());
    write_page(new_root_id, bpm->disk_manager, new_root->serialize());
}

BTreePage *BTree::findLeaf(const BTreeKey &key, std::stack<BREADCRUMB_TYPE> &breadcrumbs, page_id_t *found_pid) {
    BpmPage *curr_bpm_page = fetch_bpm_page(root_pid, bpm);
    auto root = new BTreePage(curr_bpm_page->data);
    auto temp = root;

    while (!temp->is_leaf) {
        auto first_key = temp->keys[0];
        if (BTreePage::cmpKeys(first_key.data, key.data, first_key.length, key.length) < 0) {
            breadcrumbs.push(BREADCRUMB_TYPE(temp, 0));
            auto next_ptr = std::get<internal_pointers>(temp->children).at(0);
            curr_bpm_page = fetch_bpm_page(next_ptr, bpm);
            temp = new BTreePage(curr_bpm_page->data); // TODO: FIX LEAK
            continue;
        }
        for (u16 i = 1; i < temp->keys.size(); i++) {
            auto prev_key = temp->keys[i - 1];
            auto curr_key = temp->keys[i];
            if (BTreePage::cmpKeys(prev_key.data, key.data, prev_key.length, key.length) > 0 &&
                BTreePage::cmpKeys(curr_key.data, key.data, curr_key.length, key.length) <= 0) {
                breadcrumbs.push(BREADCRUMB_TYPE(temp, i));
                auto next_ptr = std::get<internal_pointers>(temp->children).at(i);
                curr_bpm_page = fetch_bpm_page(next_ptr, bpm);
                temp = new BTreePage(curr_bpm_page->data); // TODO: FIX LEAK
                continue;
            }
        }

        breadcrumbs.push(BREADCRUMB_TYPE(temp, -1));
        curr_bpm_page = fetch_bpm_page(temp->rightmost_ptr, bpm);
        temp = new BTreePage(curr_bpm_page->data); // TODO: FIX LEAK
    }

    breadcrumbs.push(BREADCRUMB_TYPE(temp, 1));

    *found_pid =
        *reinterpret_cast<page_id_t *>(hash_find(std::to_string(curr_bpm_page->id).data(), bpm->page_table)->data);
    //*found_pid = curr_bpm_page->id;
    return temp;
}

template <typename VAL_T> void BTreePage::insertIntoNode(const BTreeKey &key, VAL_T val) {
    std::vector<VAL_T> node_values;
    if constexpr (std::is_same_v<VAL_T, RID>)
        node_values = LEAF_RECORDS(records);
    else
        node_values = INTERNAL_CHILDREN(children);

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
        new_keys.reserve(size);
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

    if constexpr (std::is_same_v<VAL_T, RID>)
        records = node_values;
    else
        children = node_values;
    return;
}

} // namespace somedb
