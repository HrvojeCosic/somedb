/*
 * The index file starts with the metadata page of the following format:
 * (32 bit uint) magic number, expected to equal BTREE_INDEX
 * (16 bit uint) current number of nodes, not including root
 * (8 bit uint) max number of keys in a node (max size/node's degree - 1)
 * (32 bit uint) page id  of the current root node
 *
 * In memory representation is in BTree struct
 */

#pragma once
#include "bpm.h"
#include "index_page.hpp"
#include "shared.h"
#include <cassert>
#include <memory>
#include <string>

/*
 * Type used for walking back up the tree after reaching leaf node,
 * containing parent pointer(on disk representation) and index in pointer array
 * index is -1 if the node was pointed to by the rightmost_ptr of the parent
 */
#define BREADCRUMB_TYPE std::pair<page_id_t, int>

namespace somedb {

using leaf_records = std::vector<RID>;
using internal_pointers = std::vector<u32>;

struct BTree {
    u32 magic_num;
    u8 max_size;
    BufferPoolManager *bpm;
    page_id_t root_pid;
    u16 node_count;

    //--------------------------------------------------------------------------------------------------------------------------------
    BTree(BufferPoolManager *bpm) : bpm(bpm) { assert(bpm->disk_manager != nullptr); }

    u8 *serialize() const;

    void deserialize();

    bool getValues(const BTreeKey &key, std::vector<RID> &result);

    // Inserts element into the tree and returns page id of leaf the element is inserted in if successful, or 0 if
    // provided key already exists
    page_id_t insert(const BTreeKey &key, const RID &val);

    //--------------------------------------------------------------------------------------------------------------------------------
  private:
    void initializeTree();

    BTreePage *findLeaf(const BTreeKey &key, std::stack<BREADCRUMB_TYPE> &breadcrumbs, page_id_t *found_pid);

    // Splits provided root node and creates a new tree root
    TREE_NODE_FUNC_TYPE void splitRootNode(BTreePage &curr_root);

    // Splits provided leaf node and propagates the split up the tree if necessary
    TREE_NODE_FUNC_TYPE void splitNonRootNode(BTreePage *old_node, const page_id_t old_node_pid,
                                              std::stack<BREADCRUMB_TYPE> &breadcrumbs);

    // Redistributes keys/values between node1 and node2
    TREE_NODE_FUNC_TYPE inline static void redistribute_kv(BTreePage *node1, BTreePage *node2) {
        bool is_leaf = node1->is_leaf && node2->is_leaf;

        node1->keys = BTreePage::vec_second_half(node2->keys, is_leaf);
        node2->keys = BTreePage::vec_first_half(node2->keys, is_leaf);
        std::vector<VAL_T> full;

        if constexpr (std::same_as<VAL_T, RID>)
            full = LEAF_RECORDS(node2->values);
        else
            full = INTERNAL_CHILDREN(node2->values);

        node1->values = BTreePage::vec_second_half<VAL_T>(full);
        node2->values = BTreePage::vec_first_half<VAL_T>(full);
    }

    // Updates provided node's contents(data) in the buffer pool and flushes it to disk
    inline static void flush_node(page_id_t node_pid, u8 *data, BufferPoolManager *bpm) {
        auto bpm_page = fetch_bpm_page(node_pid, bpm);
        frame_id_t *fid =
            static_cast<frame_id_t *>(hash_find(std::to_string(bpm_page->id).data(), bpm->page_table)->data);
        write_to_frame(*fid, data, bpm);
        flush_page(node_pid, bpm);
    }

    // Creates a BTreePage object from the breadcrumb at the top of the provided breadcrumbs stack and returns a pointer
    // to it, or null if stack is empty
    inline BTreePage *getPrevBreadcrumbPage(BTreePage &parent, std::stack<BREADCRUMB_TYPE> &breadcrumbs) {
        if (breadcrumbs.empty())
            return nullptr;

        auto top_key = breadcrumbs.top().second;
        breadcrumbs.pop();
        page_id_t pid = top_key == -1 ? parent.rightmost_ptr : top_key;
        return new BTreePage(fetch_bpm_page(pid, bpm)->data);
    }

    //--------------------------------------------------------------------------------------------------------------------------------
};

} // namespace somedb
