#include "../include/index.hpp"
#include <algorithm>
#include <cstring>
#include <iostream>
#include <stack>

namespace somedb {

BTree::BTree(std::string index_name, BufferPoolManager *bpm, u16 max_size, BTreePage *root) {
    this->index_name = index_name;
    this->bpm = bpm;

    if (root == nullptr)
        this->root = new BTreePage(true, max_size);
}

BTreePage::BTreePage(bool is_leaf, u16 max_size) {
    this->is_leaf = is_leaf;
    this->max_size = max_size;
    keys.reserve(max_size);
    if (is_leaf)
        std::get<leaf_records>(records).reserve(max_size);
    else
        std::get<internal_pointers>(children).reserve(max_size);
}

bool BTree::getValues(const BTreeKey &key, std::vector<RID> &result) { return false; }

bool BTree::insert(const BTreeKey &key, const RID &val) {
    auto *leaf = findLeaf(key);
    if (leaf->max_size > leaf->keys.size()) { // Found leaf has enough space
        insertIntoLeaf(leaf, key, val);
        return true;
    }

    return false;
}

BTreePage *BTree::findLeaf(const BTreeKey &key) {
    std::stack<BREADCRUMB_TYPE> breadcrumbs;
    BTreePage *temp = this->root;

    while (!temp->is_leaf) {
        auto first_key = temp->keys[0];
        if (cmpKeys(first_key.data, key.data, first_key.length, key.length) < 0) {
            breadcrumbs.push(BREADCRUMB_TYPE(temp, 0));
            temp = std::get<internal_pointers>(temp->children).at(0);
            continue;
        }
        for (u16 i = 1; i < temp->keys.size(); i++) {
            auto prev_key = temp->keys[i - 1];
            auto curr_key = temp->keys[i];
            if (cmpKeys(prev_key.data, key.data, prev_key.length, key.length) > 0 &&
                cmpKeys(curr_key.data, key.data, curr_key.length, key.length) <= 0) {
                breadcrumbs.push(BREADCRUMB_TYPE(temp, i));
                temp = std::get<internal_pointers>(temp->children).at(i);
                continue;
            }
        }

        breadcrumbs.push(BREADCRUMB_TYPE(temp, -1));
        temp = std::get<internal_pointers>(temp->children).back(); // set it to rightmost child pointer
    }

    return temp;
}

void BTree::insertIntoLeaf(BTreePage *node, const BTreeKey &key, const RID &val) {
    uint16_t size = node->keys.size();
    node->keys.resize(size + 1);
    LEAF_RECORDS(node->records).resize(size + 1);

    // New key is larger (or equal) than all current ones
    auto last_key = node->keys.at(node->keys.size() - 1);
    if (cmpKeys(key.data, last_key.data, key.length, last_key.length) >= 0) {
        node->keys.at(node->keys.size() - 1) = key;
        auto y = LEAF_RECORDS(node->records);
        LEAF_RECORDS(node->records).at(LEAF_RECORDS(node->records).size() - 1) = val;
        return;
    }
    // New key is smaller (or equal) than all current ones
    auto first_key = node->keys.at(0);
    if (cmpKeys(key.data, first_key.data, key.length, first_key.length) <= 0) {
        node->keys.at(0) = key;
        LEAF_RECORDS(node->records).at(0) = val;
        return;
    }

    // New key is in the middle.
    // Insert it at appropriate index and shift all following elements to right
    for (uint16_t i = 0; i < size; i++) {
        auto curr_key = node->keys.at(i);
        auto next_key = node->keys.at(i + 1);
        if (cmpKeys(key.data, curr_key.data, key.length, curr_key.length) > 0 &&
            cmpKeys(key.data, next_key.data, key.length, next_key.length) <= 0) {
            // shift all elements to right (for both keys and values)
            BTreeKey prev_key = node->keys.at(i + 1);
            RID prev_value = std::get<leaf_records>(node->records).at(i + 1);
            node->keys.at(i + 1) = key;
            std::get<leaf_records>(node->records).at(i + 1) = val;
            for (int j = i; j < size - 1; j++) {
                BTreeKey temp_key = node->keys.at(j + 2);
                RID temp_val = LEAF_RECORDS(node->records).at(j + 2);
                // RID temp_val = std::get<leaf_records>(node->records).at(j+2);
                node->keys.at(j + 2) = prev_key;
                std::get<leaf_records>(node->records).at(j + 2) = prev_value;
                prev_key = temp_key;
                prev_value = temp_val;
            }
            break;
        }
    }
}

} // namespace somedb
