#include "../include/btree_index.hpp"
#include "../include/disk_manager.h"
#include "../include/heapfile.h"
#include "../include/index_page.hpp"
#include "../include/serialize.h"
#include <algorithm>
#include <bits/ranges_util.h>
#include <cassert>
#include <cstring>
#include <iostream>
#include <iterator>
#include <memory>
#include <stack>
#include <stdexcept>
#include <unistd.h>

namespace somedb {

u8 *BTree::serialize() const {
    u8 *data = new u8[PAGE_SIZE];

    encode_uint32(magic_num, data + MAGIC_NUMBER_OFFSET);
    encode_uint16(node_count, data + NODE_COUNT_OFFSET);
    encode_uint32(root_pid, data + CURR_ROOT_PID_OFFSET);
    memcpy(data + MAX_KEYSIZE_OFFSET, &max_size, sizeof(u8));

    return data;
}

void BTree::deserialize() {
    u8 *metadata = read_page(BTREE_METADATA_PAGE_ID, bpm->disk_manager);

    magic_num = decode_uint32(metadata + MAGIC_NUMBER_OFFSET);
    assert(magic_num == BTREE_INDEX);
    node_count = decode_uint16(metadata + NODE_COUNT_OFFSET);
    root_pid = decode_uint32(metadata + CURR_ROOT_PID_OFFSET);
    max_size = *(metadata + MAX_KEYSIZE_OFFSET);
}

page_id_t BTree::insert(const BTreeKey &key, const RID &val) {
    if (root_pid <= BTREE_METADATA_PAGE_ID) {
        root_pid = new_btree_index_page(bpm->disk_manager, true);
        assert(root_pid != BTREE_METADATA_PAGE_ID);
        node_count++;
    }

    auto breadcrumbs = std::stack<BREADCRUMB_TYPE>();
    page_id_t leaf_pid = 0;
    auto leaf = findLeaf(key, breadcrumbs, leaf_pid);
    assert(leaf_pid != BTREE_METADATA_PAGE_ID);

    // return early if duplicate key
    for (BTreeKey leaf_key : leaf->keys)
        if (BTreePage::cmpKeys(leaf_key, key) == 0)
            return 0;

    leaf->insertIntoNode<RID>(key, val);

    if (max_size < leaf->keys.size()) {
        assert(root_pid != BTREE_METADATA_PAGE_ID);
        if (breadcrumbs.size() == 0)
            splitRootNode<RID>(leaf);
        else
            splitNonRootNode<RID>(leaf, leaf_pid, breadcrumbs);
    }

    flush_node(leaf_pid, leaf->serialize(), bpm);
    return leaf_pid;
}

TREE_NODE_FUNC_TYPE void BTree::splitNonRootNode(std::unique_ptr<BTreePage> &old_node, const page_id_t old_node_pid,
                                                 std::stack<BREADCRUMB_TYPE> &breadcrumbs) {
    auto mid_key = old_node->keys.at(old_node->keys.size() / 2);
    constexpr bool is_node_leaf = std::same_as<VAL_T, RID>;

    auto new_node = std::make_unique<BTreePage>(is_node_leaf);
    const page_id_t new_node_id = new_btree_index_page(bpm->disk_manager, is_node_leaf);
    node_count++;

    redistribute_kv<VAL_T>(new_node, old_node);

    // update sibling pointers
    new_node->next = old_node->next;
    old_node->next = new_node_id;

    // update parent
    auto parent_pid = getPrevBreadcrumbPid(breadcrumbs);
    auto parent = std::make_unique<BTreePage>(fetch_bpm_page(parent_pid, bpm)->data);
    parent->insertIntoNode(mid_key, old_node_pid);
    if (parent->keys.at(parent->keys.size() - 1) == mid_key)
        parent->rightmost_ptr = new_node_id;

    // persist to disk
    assert(new_node_id != BTREE_METADATA_PAGE_ID && old_node_pid != BTREE_METADATA_PAGE_ID);
    flush_node(parent_pid, parent->serialize(), bpm);
    flush_node(new_node_id, new_node->serialize(), bpm);
    flush_node(old_node_pid, old_node->serialize(), bpm);

    // call for other ascendants if necessary
    if (parent->keys.size() > max_size) {
        if (breadcrumbs.empty()) {
            std::unique_ptr<BTreePage> root_page = std::make_unique<BTreePage>(fetch_bpm_page(root_pid, bpm)->data);
            splitRootNode<u32>(root_page);
        } else {
            auto top_pid = getPrevBreadcrumbPid(breadcrumbs);
            auto top_node = std::make_unique<BTreePage>(fetch_bpm_page(top_pid, bpm)->data);
            splitNonRootNode<u32>(top_node, top_pid, breadcrumbs);
        }
    }
}

TREE_NODE_FUNC_TYPE void BTree::splitRootNode(std::unique_ptr<BTreePage> &old_root_node) {
    auto old_root_pid = root_pid;
    auto mid_key = old_root_node->keys.at(old_root_node->keys.size() / 2);
    constexpr bool is_old_root_leaf = std::same_as<VAL_T, RID>;

    auto new_node_id = new_btree_index_page(bpm->disk_manager, is_old_root_leaf);
    auto new_root_id = new_btree_index_page(bpm->disk_manager, false);
    auto new_node = std::make_unique<BTreePage>(is_old_root_leaf);
    auto new_root = std::make_unique<BTreePage>(false);
    node_count += 2;

    redistribute_kv<VAL_T>(new_node, old_root_node);

    old_root_node->next = new_node_id;

    // create new root and connect nodes to it
    new_root->keys.insert(new_root->keys.begin(), mid_key);
    auto &children = INTERNAL_CHILDREN(new_root->values);
    children.insert(children.begin(), root_pid);
    new_root->rightmost_ptr = new_node_id;
    root_pid = new_root_id;

    // persist all changes
    assert(new_node_id != BTREE_METADATA_PAGE_ID && new_root_id != BTREE_METADATA_PAGE_ID);
    flush_node(BTREE_METADATA_PAGE_ID, serialize(), bpm);
    flush_node(new_node_id, new_node->serialize(), bpm);
    flush_node(root_pid, new_root->serialize(), bpm);
    flush_node(old_root_pid, old_root_node->serialize(), bpm);
}

void BTree::remove(const BTreeKey &key) {
    auto breadcrumbs = std::stack<BREADCRUMB_TYPE>();
    page_id_t leaf_pid = 0;
    auto leaf = findLeaf(key, breadcrumbs, leaf_pid);
    auto &leaf_vals = LEAF_RECORDS(leaf->values);

    uint i;
    for (i = 0; i < leaf->keys.size(); i++)
        if (leaf->keys.at(i) == key)
            break;

    leaf->keys.erase(leaf->keys.begin() + i);
    leaf_vals.erase(leaf_vals.begin() + i);
    flush_node(leaf_pid, leaf->serialize(), bpm);

    merge<RID>(leaf, leaf_pid, breadcrumbs);
}

TREE_NODE_FUNC_TYPE void BTree::merge(std::unique_ptr<BTreePage> &node, const page_id_t node_pid, std::stack<BREADCRUMB_TYPE> &breadcrumbs) {
    int parent_link = breadcrumbs.top().second;
    auto parent_pid = getPrevBreadcrumbPid(breadcrumbs);
    auto parent = std::make_unique<BTreePage>(fetch_bpm_page(parent_pid, bpm)->data);
    auto parent_vals = INTERNAL_CHILDREN(parent->values);

    // We choose the right sibling by default, unless the current node is a parent's rightmost pointer
    bool is_left_sib = parent_link == -1;
    auto sibling_pid = is_left_sib ? parent_vals.at(parent_vals.size() - 1) : node->next;
    auto sibling = std::make_unique<BTreePage>(fetch_bpm_page(sibling_pid, bpm)->data);
    auto node_vals = getValues<VAL_T>(node);

    if (sibling_pid && sibling->keys.size() + node->keys.size() <= max_size) {
        auto &sibling_vals = getValues<VAL_T>(sibling);
        if (is_left_sib) {
            sibling->keys.insert(sibling->keys.end(), node->keys.begin(), node->keys.end());
            sibling_vals.insert(sibling_vals.end(), node_vals.begin(), node_vals.end());
            sibling->rightmost_ptr = node->rightmost_ptr;
            flush_node(sibling_pid, sibling->serialize(), bpm);

            parent->rightmost_ptr = sibling_pid;
            INTERNAL_CHILDREN(parent->values).pop_back();
            flush_node(parent_pid, parent->serialize(), bpm);
        } else {
            node->keys.insert(node->keys.end(), sibling->keys.begin(), sibling->keys.end());
            node_vals.insert(node_vals.end(), sibling_vals.begin(), sibling_vals.end());
            flush_node(node_pid, node->serialize(), bpm);

            parent_vals.erase(parent_vals.begin() + parent_link);
            parent->keys.erase(parent->keys.begin() + parent_link);
            flush_node(parent_pid, parent->serialize(), bpm);
        }
    }
}

std::unique_ptr<BTreePage> BTree::findLeaf(const BTreeKey &key, std::stack<BREADCRUMB_TYPE> &breadcrumbs,
                                           page_id_t &found_pid) {
    auto *curr_bpm_page = fetch_bpm_page(root_pid, bpm);
    auto temp = std::make_unique<BTreePage>(curr_bpm_page->data);

    while (!temp->is_leaf) {
        auto first_key = temp->keys.at(0);
        // provided key is smaller than first existing key
        if (BTreePage::cmpKeys(first_key, key) > 0) {
            breadcrumbs.push(BREADCRUMB_TYPE(curr_bpm_page->id, 0));
            auto next_ptr = std::get<internal_pointers>(temp->values).at(0);
            curr_bpm_page = fetch_bpm_page(next_ptr, bpm);
            temp = std::make_unique<BTreePage>(curr_bpm_page->data);
            continue;
        }
        for (u16 i = 1; i < temp->keys.size(); i++) {
            auto prev_key = temp->keys.at(i - 1);
            auto curr_key = temp->keys.at(i);
            if (BTreePage::cmpKeys(prev_key, key) > 0 && BTreePage::cmpKeys(curr_key, key) <= 0) {
                breadcrumbs.push(BREADCRUMB_TYPE(curr_bpm_page->id, i));
                auto next_ptr = std::get<internal_pointers>(temp->values).at(i);
                curr_bpm_page = fetch_bpm_page(next_ptr, bpm);
                temp = std::make_unique<BTreePage>(curr_bpm_page->data);
                continue;
            }
        }

        // provided key is bigger than rightmost pointer
        breadcrumbs.push(BREADCRUMB_TYPE(curr_bpm_page->id, -1));
        curr_bpm_page = fetch_bpm_page(temp->rightmost_ptr, bpm);
        temp = std::make_unique<BTreePage>(curr_bpm_page->data);
    }

    found_pid = curr_bpm_page->id;
    return temp;
}

} // namespace somedb
