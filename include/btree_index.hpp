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
#include "index_page.hpp"

/*
 * Type used for walking back up the tree after reaching leaf node,
 * containing parent pointer and index in pointer array
 * index is -1 if the node was pointed to by the rightmost_ptr of the parent
 */
#define BREADCRUMB_TYPE std::pair<BTreePage *, int>

/* Serialization sizes and offsets */
#define BTREE_METADATA_PAGE_ID 0
#define MAGIC_NUM_SIZE 4
#define MAGIC_NUMBER_OFFSET 0
#define NODE_COUNT_SIZE 2
#define NODE_COUNT_OFFSET MAGIC_NUM_SIZE
#define MAX_KEYSIZE_SIZE 1
#define MAX_KEYSIZE_OFFSET MAGIC_NUM_SIZE + NODE_COUNT_SIZE
#define CURR_ROOT_PID_SIZE 4
#define CURR_ROOT_PID_OFFSET MAGIC_NUM_SIZE + NODE_COUNT_SIZE + MAX_KEYSIZE_SIZE

namespace somedb {

using leaf_records = std::vector<RID>;
using internal_pointers = std::vector<u32>;

struct BTree {
    const u32 magic_num;
    const u8 max_size;
    BufferPoolManager *bpm;
    page_id_t root_pid;
    u16 node_count;
    //--------------------------------------------------------------------------------------------------------------------------------
    BTree(u8 data[PAGE_SIZE], BufferPoolManager *bpm);

    u8 *serialize() const;

    bool getValues(const BTreeKey &key, std::vector<RID> &result);

    /* Insert element into the tree and return true if successful, or false if provided key already exists */
    bool insert(const BTreeKey &key, const RID &val);

    //--------------------------------------------------------------------------------------------------------------------------------
  private:
    BTreePage *findLeaf(const BTreeKey &key, std::stack<BREADCRUMB_TYPE> &breadcrumbs, page_id_t *found_pid);

    void splitRootNode();
    //--------------------------------------------------------------------------------------------------------------------------------
};

} // namespace somedb
