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
    std::unique_ptr<BTreePage> findLeaf(const BTreeKey &key, std::stack<BREADCRUMB_TYPE> &breadcrumbs,
                                        page_id_t *found_pid);

    // Splits provided root node and creates a new tree root
    TREE_NODE_FUNC_TYPE void splitRootNode(std::unique_ptr<BTreePage> &curr_root_node);

    // Splits provided leaf node and propagates the split up the tree if necessary
    TREE_NODE_FUNC_TYPE void splitNonRootNode(std::unique_ptr<BTreePage> &old_node, const page_id_t old_node_pid,
                                              std::stack<BREADCRUMB_TYPE> &breadcrumbs);

    // Redistributes keys/values between provided nodes.
    // Currently made specifically for splitting nodes, will be modified if needed for other operations
    TREE_NODE_FUNC_TYPE inline static void redistribute_kv(std::unique_ptr<BTreePage> &empty_node,
                                                           std::unique_ptr<BTreePage> &full_node) {
        empty_node->keys = BTreePage::vec_second_half(full_node->keys, empty_node->is_leaf);
        full_node->keys = BTreePage::vec_first_half(full_node->keys);

        std::vector<VAL_T> full_vals;
        if constexpr (std::same_as<VAL_T, RID>) {
            full_vals = LEAF_RECORDS(full_node->values);
            empty_node->values = BTreePage::vec_second_half<VAL_T>(full_vals);
            full_node->values = BTreePage::vec_first_half<VAL_T>(full_vals);
        } else {
            full_vals = INTERNAL_CHILDREN(full_node->values);
            empty_node->values = BTreePage::vec_second_half<VAL_T>(full_vals, false);
            full_node->values = BTreePage::vec_first_half<VAL_T>(full_vals, true);

            // full_node is basically the root node before splitting which goes to the left of the root after splitting,
            // which means we need to move its rightmost pointer to the new node which goes to the right of the root
            // after splitting, and since we now have same number of keys as values in the full_node, make its last
            // value the rightmost pointer instead
            empty_node->rightmost_ptr = full_node->rightmost_ptr;

            full_vals = INTERNAL_CHILDREN(full_node->values);
            full_node->rightmost_ptr = full_vals.at(full_vals.size() - 1);
            full_vals.pop_back();
        }
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
    inline std::unique_ptr<BTreePage> getPrevBreadcrumbPage(std::unique_ptr<BTreePage> parent,
                                                            std::stack<BREADCRUMB_TYPE> &breadcrumbs) {
        if (breadcrumbs.empty())
            return nullptr;

        auto top_key = breadcrumbs.top().second;
        breadcrumbs.pop();
        page_id_t pid = top_key == -1 ? parent->rightmost_ptr : top_key;
        return std::make_unique<BTreePage>(fetch_bpm_page(pid, bpm)->data);
    }

    //--------------------------------------------------------------------------------------------------------------------------------
};

} // namespace somedb
