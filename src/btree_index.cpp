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
#include <string>
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

page_id_t BTree::insert(const BTreeKey &key, const RID &val) {
    if (root_pid <= BTREE_METADATA_PAGE_ID) {
        root_pid = new_btree_index_page(bpm->disk_manager);
        assert(root_pid != BTREE_METADATA_PAGE_ID);
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
        if (leaf->level == 0)
            splitRootNode();
        else
            throw std::runtime_error("NON-ROOT SPLITS NOT YET IMPLEMENTED"); // TODO
    }

    auto bpm_leaf = fetch_bpm_page(leaf_pid, bpm);
    frame_id_t *bpm_fid =
        static_cast<frame_id_t *>(hash_find(std::to_string(bpm_leaf->id).data(), bpm->page_table)->data);
    write_to_frame(*bpm_fid, leaf->serialize(), bpm);
    flush_page(bpm_leaf->id, bpm);

    return leaf_pid;
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
    assert(new_node_id != BTREE_METADATA_PAGE_ID && new_root_id != BTREE_METADATA_PAGE_ID);
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
    *found_pid = curr_bpm_page->id;
    return temp;
}

} // namespace somedb
