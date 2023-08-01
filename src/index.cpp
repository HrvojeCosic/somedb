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
    keys.reserve(max_size + 1); // Logically, maximum size is still max_size, but +1 simplifies splitting process by
                                // being able to insert the new element before the split happens, so new element doesn't
                                // have to be accounted for seperately in that process
    if (is_leaf) {
        records = leaf_records();
        std::get<leaf_records>(records).reserve(max_size);
    } else {
        children = internal_pointers();
        std::get<internal_pointers>(children).reserve(max_size + 1); // +1 is for the rightmost child pointer
    }
}

bool BTree::getValues(const BTreeKey &key, std::vector<RID> &result) { return false; }

bool BTree::insert(const BTreeKey &key, const RID &val) {
    auto breadcrumbs = std::stack<BREADCRUMB_TYPE>();
    BTreePage *leaf = findLeaf(key, breadcrumbs);

    // return early if duplicate key
    for (BTreeKey leaf_key : leaf->keys)
        if (cmpKeys(leaf_key.data, key.data, leaf_key.length, key.length) == 0)
            return false;

    insertIntoNode<RID>(leaf, key, val);

    if (leaf->max_size < leaf->keys.size()) {
        if (leaf == root)
            splitRootNode(leaf);
        else
            throw std::runtime_error("NON-ROOT SPLITS NOT YET IMPLEMENTED"); // TODO
    }

    return true;
}

void BTree::splitRootNode(BTreePage *node) {
    auto mid_key = node->keys.at(node->keys.size() / 2);

    auto new_node = new BTreePage(true, node->max_size);
    auto new_root = new BTreePage(false, node->max_size);

    // redistribute keys/values between old and new node
    new_node->keys = vec_second_half<BTreeKey>(node->keys);
    node->keys = vec_first_half(node->keys);
    auto full = LEAF_RECORDS(node->records);
    new_node->records = vec_second_half<RID>(full);
    node->records = vec_first_half<RID>(full);

    new_node->previous = node;
    node->next = new_node;

    // connect nodes to new root
    new_root->keys.insert(new_root->keys.begin(), mid_key);
    INTERNAL_CHILDREN(new_root->children).insert(INTERNAL_CHILDREN(new_root->children).begin(), node);
    INTERNAL_CHILDREN(new_root->children).insert(INTERNAL_CHILDREN(new_root->children).begin() + 1, new_node);
    this->root = new_root;
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

        breadcrumbs.push(BREADCRUMB_TYPE(temp, -1)); // set it to the rightmost child pointer
        temp = std::get<internal_pointers>(temp->children).back();
    }

    breadcrumbs.push(BREADCRUMB_TYPE(temp, 1));
    return temp;
}

TREE_NODE_FUNC_TYPE
void BTree::insertIntoNode(BTreePage *node, const BTreeKey &key, VAL_T val) {
    std::vector<VAL_T> node_values;
    if constexpr (std::is_same_v<VAL_T, RID>)
        node_values = LEAF_RECORDS(node->records);
    else
        node_values = INTERNAL_CHILDREN(node->children);

    uint16_t size = node->keys.size();
    node->keys.resize(size + 1);
    node_values.resize(size + 1);
    auto first_key = node->keys.at(0);
    auto last_key = node->keys.at(size == 0 ? size : size - 1);

    // New key is larger (or equal) than all current ones
    if (cmpKeys(key.data, last_key.data, key.length, last_key.length) >= 0) {
        node->keys.at(node->keys.size() - 1) = key;
        node_values.at(node_values.size() - 1) = val;
    }
    // New key is smaller (or equal) than all current ones
    else if (cmpKeys(key.data, first_key.data, key.length, first_key.length) <= 0) {
        std::vector<BTreeKey> new_keys;
        new_keys.reserve(size);
        new_keys.push_back(key);
        new_keys.insert(new_keys.end(), node->keys.begin(), node->keys.end() - 1);
        node->keys = new_keys;

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
                    VAL_T temp_val = node_values.at(j + 2);
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
