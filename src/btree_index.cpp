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
#include <memory>
#include <stack>
#include <stdexcept>
#include <unistd.h>

namespace somedb {

u8 *BTree::serialize() const {
    u8 *data = new u8[PAGE_SIZE];

    encode_uint32(this->magic_num, data + MAGIC_NUMBER_OFFSET);
    encode_uint16(this->node_count, data + NODE_COUNT_OFFSET);
    encode_uint32(this->root_pid, data + CURR_ROOT_PID_OFFSET);
    memcpy(data + MAX_KEYSIZE_OFFSET, &this->max_size, sizeof(u8));

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

inline void BTree::flush_node(page_id_t node_pid, u8 *data) {
    auto bpm_page = fetch_bpm_page(node_pid, bpm);
    frame_id_t *fid = static_cast<frame_id_t *>(hash_find(std::to_string(bpm_page->id).data(), bpm->page_table)->data);
    write_to_frame(*fid, data, bpm);
    flush_page(node_pid, bpm);
}

page_id_t BTree::insert(const BTreeKey &key, const RID &val) {
    if (root_pid <= BTREE_METADATA_PAGE_ID) {
        root_pid = new_btree_index_page(bpm->disk_manager, true);
        assert(root_pid != BTREE_METADATA_PAGE_ID);
        node_count++;
    }

    auto breadcrumbs = std::stack<BREADCRUMB_TYPE>();
    page_id_t leaf_pid = 0;
    auto leaf = findLeaf(key, breadcrumbs, &leaf_pid);
    assert(leaf_pid != BTREE_METADATA_PAGE_ID);

    // return early if duplicate key
    for (BTreeKey leaf_key : leaf->keys)
        if (BTreePage::cmpKeys(leaf_key.data, key.data, leaf_key.length, key.length) == 0)
            return 0;

    leaf->insertIntoNode<RID>(key, val);

    if (this->max_size < leaf->keys.size()) {
        assert(root_pid != BTREE_METADATA_PAGE_ID);
        if (breadcrumbs.size() == 0)
            splitRootNode(leaf);
        else
            splitLeafNode(leaf, leaf_pid, breadcrumbs);
    }

    flush_node(leaf_pid, leaf->serialize());
    return leaf_pid;
}

void BTree::splitLeafNode(BTreePage *old_node, const page_id_t old_node_pid, std::stack<BREADCRUMB_TYPE> &breadcrumbs) {
    auto mid_key = old_node->keys.at(old_node->keys.size() / 2);

    auto new_node = new BTreePage(true);
    const page_id_t new_node_id = new_btree_index_page(bpm->disk_manager, true);

    // redistribute (todo: pull out to a separate function and use it for splitRootNode too)
    new_node->keys = BTreePage::vec_second_half<BTreeKey>(old_node->keys);
    old_node->keys = BTreePage::vec_first_half(old_node->keys);
    auto full = LEAF_RECORDS(old_node->values);
    new_node->values = BTreePage::vec_second_half<RID>(full);
    old_node->values = BTreePage::vec_first_half<RID>(full);

    // update sibling pointers
    new_node->next = old_node->next;
    old_node->next = new_node_id;

    // update parent
    BREADCRUMB_TYPE prev_crumb = breadcrumbs.top();
    breadcrumbs.pop();
    auto parent_bpm_page = fetch_bpm_page(prev_crumb.first, bpm);
    auto parent = new BTreePage(parent_bpm_page->data);
    parent->insertIntoNode(mid_key, old_node_pid);
    if (parent->keys.at(parent->keys.size() - 1) == mid_key)
        parent->rightmost_ptr = new_node_id;

    // persist to disk
    assert(new_node_id != BTREE_METADATA_PAGE_ID && old_node_pid != BTREE_METADATA_PAGE_ID);
    flush_node(parent_bpm_page->id, parent->serialize());
    flush_node(new_node_id, new_node->serialize());
    flush_node(old_node_pid, old_node->serialize());

    // call for other ascendants if necessary
    // TODO
}

void BTree::splitRootNode(BTreePage *old_root_node) {
    auto mid_key = old_root_node->keys.at(old_root_node->keys.size() / 2);

    auto new_node_id = new_btree_index_page(bpm->disk_manager, true);
    auto new_root_id = new_btree_index_page(bpm->disk_manager, false);
    auto new_node = new BTreePage(true);
    auto new_root = new BTreePage(false);
    node_count += 2;

    // redistribute keys/values between old and new root node
    new_node->keys = BTreePage::vec_second_half<BTreeKey>(old_root_node->keys);
    old_root_node->keys = BTreePage::vec_first_half(old_root_node->keys);
    auto full = LEAF_RECORDS(old_root_node->values);
    new_node->values = BTreePage::vec_second_half<RID>(full);
    old_root_node->values = BTreePage::vec_first_half<RID>(full);

    old_root_node->next = new_node_id;

    // create new root and connect nodes to it
    new_root->keys.insert(new_root->keys.begin(), mid_key);
    auto &children = INTERNAL_CHILDREN(new_root->values);
    children.insert(children.begin(), root_pid);
    new_root->rightmost_ptr = new_node_id;
    root_pid = new_root_id;

    // persist all changes
    assert(new_node_id != BTREE_METADATA_PAGE_ID && new_root_id != BTREE_METADATA_PAGE_ID);
    flush_node(BTREE_METADATA_PAGE_ID, serialize());
    flush_node(new_node_id, new_node->serialize());
    flush_node(new_root_id, new_root->serialize());
}

BTreePage *BTree::findLeaf(const BTreeKey &key, std::stack<BREADCRUMB_TYPE> &breadcrumbs, page_id_t *found_pid) {
    auto *curr_bpm_page = fetch_bpm_page(root_pid, bpm);
    auto root = new BTreePage(curr_bpm_page->data);
    auto temp = root;

    while (!temp->is_leaf) {
        auto first_key = temp->keys.at(0);
        // provided key is smaller than first existing key
        if (BTreePage::cmpKeys(first_key.data, key.data, first_key.length, key.length) > 0) {
            breadcrumbs.push(BREADCRUMB_TYPE(curr_bpm_page->id, 0));
            auto next_ptr = std::get<internal_pointers>(temp->values).at(0);
            curr_bpm_page = fetch_bpm_page(next_ptr, bpm);
            temp = new BTreePage(curr_bpm_page->data);
            continue;
        }
        for (u16 i = 1; i < temp->keys.size(); i++) {
            auto prev_key = temp->keys.at(i - 1);
            auto curr_key = temp->keys.at(i);
            if (BTreePage::cmpKeys(prev_key.data, key.data, prev_key.length, key.length) > 0 &&
                BTreePage::cmpKeys(curr_key.data, key.data, curr_key.length, key.length) <= 0) {
                breadcrumbs.push(BREADCRUMB_TYPE(curr_bpm_page->id, i));
                auto next_ptr = std::get<internal_pointers>(temp->values).at(i);
                curr_bpm_page = fetch_bpm_page(next_ptr, bpm);
                temp = new BTreePage(curr_bpm_page->data);
                continue;
            }
        }

        // provided key is bigger than rightmost pointer
        breadcrumbs.push(BREADCRUMB_TYPE(curr_bpm_page->id, -1));
        curr_bpm_page = fetch_bpm_page(temp->rightmost_ptr, bpm);
        temp = new BTreePage(curr_bpm_page->data);
    }

    *found_pid = curr_bpm_page->id;
    return temp;
}

} // namespace somedb
