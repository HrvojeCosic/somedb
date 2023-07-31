#include "../include/index.hpp"
#include <algorithm>
#include <bits/ranges_util.h>
#include <cstring>
#include <iostream>
#include <stack>

namespace somedb {

using leaf_records = std::vector<RID>;

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

void BTree::insert(const BTreeKey &key, const RID &val) {
    auto breadcrumbs = std::stack<BREADCRUMB_TYPE>();
    BTreePage *leaf = findLeaf(key, breadcrumbs);
    if (leaf->max_size > leaf->keys.size()) { // Found leaf has enough space
        insertIntoNode<RID>(leaf, key, val);
        return;
    }

    splitNode(leaf, breadcrumbs);
}

void BTree::splitNode(BTreePage *node, std::stack<BREADCRUMB_TYPE> breadcrumbs) {
}

BTreePage *BTree::findLeaf(const BTreeKey &key, std::stack<BREADCRUMB_TYPE> &breadcrumbs) {
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

    breadcrumbs.push(BREADCRUMB_TYPE(temp, 1));
    return temp;
}

template <typename VAL_T>

requires(std::same_as<VAL_T, RID> ||
         std::same_as<VAL_T, BTreePage *>) void BTree::insertIntoNode(BTreePage *node, const BTreeKey &key, VAL_T val) {
    std::vector<VAL_T> node_values;
    if constexpr (std::is_same_v<VAL_T, RID>)
        node_values = std::get<std::vector<RID>>(node->records);
    else
        node_values = std::get<std::vector<BTreePage *>>(node->children);

    uint16_t size = node->keys.size();
    node->keys.resize(size + 1);
    node_values.resize(size + 1);
    auto first_key = node->keys.at(0);
    auto last_key = node->keys.at(node->keys.size() - 1);

    // New key is larger (or equal) than all current ones
    if (cmpKeys(key.data, last_key.data, key.length, last_key.length) >= 0) {
        node->keys.at(node->keys.size() - 1) = key;
        node_values.at(node_values.size() - 1) = val;
    }
    // New key is smaller (or equal) than all current ones
    else if (cmpKeys(key.data, first_key.data, key.length, first_key.length) <= 0) {
        node->keys.at(0) = key;
        node_values.at(0) = val;
        if constexpr (std::is_same_v<VAL_T, RID>)
            node->records = node_values;
        else
            node->children = node_values;
        return;
    }
    // New key is in the middle.
    // Insert it at appropriate index and shift all following elements to right
    else {
        for (uint16_t i = 0; i < size; i++) {
            auto curr_key = node->keys.at(i);
            auto next_key = node->keys.at(i + 1);
            if (cmpKeys(key.data, curr_key.data, key.length, curr_key.length) > 0 &&
                cmpKeys(key.data, next_key.data, key.length, next_key.length) <= 0) {
                // shift all elements to right (for both keys and values)
                BTreeKey prev_key = node->keys.at(i + 1);
                VAL_T prev_value = node_values.at(i + 1);
                node->keys.at(i + 1) = key;
                node_values.at(i + 1) = val;
                for (int j = i; j < size - 1; j++) {
                    BTreeKey temp_key = node->keys.at(j + 2);
                    RID temp_val = node_values.at(j + 2);
                    node->keys.at(j + 2) = prev_key;
                    node_values.at(j + 2) = prev_value;
                    prev_key = temp_key;
                    prev_value = temp_val;
                }
                break;
            }
        }
    }

    if constexpr (std::is_same_v<VAL_T, RID>)
        node->records = node_values;
    else
        node->children = node_values;
    return;
}

} // namespace somedb
