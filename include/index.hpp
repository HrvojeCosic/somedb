#pragma once

#include "bpm.h"
#include <concepts>
#include <cstdint>
#include <cstring>
#include <memory>
#include <stack>
#include <string>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

#include "./shared.h"
#include "disk_manager.h"

/*
 * Type used for walking back up the tree after reaching leaf node,
 * containing parent pointer and index in pointer array
 * index is -1 if the node was pointed to by the rightmost_ptr of the parent
 */
#define BREADCRUMB_TYPE std::pair<BTreePage *, int>

#define LEAF_RECORDS(records) std::get<std::vector<RID>>(records)
#define INTERNAL_CHILDREN(children) std::get<std::vector<u32>>(children)
#define INDEX_PAGE_HEADER_SIZE 17
#define BTREE_METADATA_PAGE_ID 0

namespace somedb {

using leaf_records = std::vector<RID>;
using internal_pointers = std::vector<u32>;

struct BTreeKey {
    u8 *data;
    u8 length;

    bool operator==(const BTreeKey &other) const {
        return (length == other.length) && (std::memcmp(data, other.data, length) == 0);
    }
};

/*
 * On disk, following the metadata page, there is a list of b+tree pages
 *
 * Each b+tree page is layed out in the slotted page format (like in heap files), with the page header including:
 *  (16 bit uint) page offset to the beginning of available space
 *  (16 bit uint) page offset to the end of available space
 *  (32 bit uint) page id of previous (sibling) page
 *  (32 bit uint) page id of next (sibling) page
 *  (32 bit uint) page id of rightmost pointer (set to 0 for leaf nodes, which is a way of identifying leafs)
 *  (16 bit uint) node's level in the tree (0 for root)
 *  (8 bit uint) special page header flags
 *
 *  Key-value pair pointers consists of:
 *  -16 bit unsigned integer representing the page offset to the corresponding kv pair
 *  -16 bit unsigned integer representing the kv pair size
 *
 *  Key-value pairs consist of:
 *  (8 bit uint) length of variable-sized key (n)
 *  (n) variable sized key
 *  (32 bit uint OR 2x32 bit uint) child pointer OR RID=(page_id, slot_num)
 */
#define IS_LEAF_SIZE 1
#define IS_LEAF_OFFSET 16

struct BTreePage {
    const bool is_leaf;
    u16 level; // starting from 0
    std::vector<BTreeKey> keys;
    u8 flags;
    page_id_t rightmost_ptr;

    // Tracking siblings for sequential scan across leaf nodes.
    // only storing their file offsets so it's not necessary to load them from disk before referencing them
    page_id_t previous;
    page_id_t next;

    /* Depending on node type, store record ids of tuples (leaf) or pointers to descendants (internal). */
    std::variant<std::vector<RID>, std::vector<u32>> records, children;
    //--------------------------------------------------------------------------------------------------------------------------------
    BTreePage(u8 data[PAGE_SIZE]);
    BTreePage(bool is_leaf) : is_leaf(is_leaf){};
    ~BTreePage();

    u8 *serialize() const;

    /* From raw bytes of DATA, determine if the page is of leaf type*/
    inline bool get_is_leaf(u8 data[PAGE_SIZE]) { return *(data + IS_LEAF_OFFSET) == 0; }

    /* If first key is bigger return >0, if second is bigger return <0, if equal return 0 */
    inline static int cmpKeys(const u8 *key1, const u8 *key2, u16 len1, u16 len2) {
        u16 len = std::min(len1, len2);
        return memcmp(key1, key2, len);
    }

    /* Get the second half of the vector (useful for things like node splits) */
    template <typename VAL_T> inline static std::vector<VAL_T> vec_second_half(std::vector<VAL_T> full_vec) {
        return {full_vec.begin() + (full_vec.size() / 2), full_vec.end()};
    }

    /* Get the first half of the vector */
    template <typename VAL_T> inline static std::vector<VAL_T> vec_first_half(std::vector<VAL_T> full_vec) {
        return {full_vec.begin(), full_vec.begin() + full_vec.size() / 2};
    }

    /*
     * Inserts provided key and value into node (leaf or internal).
     * Key/value pairs inside a node remain sorted after the insertion.
     * Provided VAL can be of two types (as seen in BTreePage), depending on if insertion is being done in leaf or
     * internal node
     */
    template <typename VAL_T> void insertIntoNode(const BTreeKey &key, VAL_T val);

    inline auto get_node_vals(BTreePage *node);
    //--------------------------------------------------------------------------------------------------------------------------------
};

/* BTree struct is an in memory representation of a metadata page stored in an index file.
 *
 * The index file starts with the metadata page of the following format:
 * (32 bit uint) magic number, expected to equal BTREE_INDEX
 * (16 bit uint) current number of nodes, not including root
 * (8 bit uint) max number of keys in a node (max size/node's degree - 1)
 * (32 bit uint) page id  of the current root node
 */
#define MAGIC_NUM_SIZE 4
#define MAGIC_NUMBER_OFFSET 0
#define CURR_NODE_NUM_SIZE 2
#define CURR_NODE_NUM_OFFSET MAGIC_NUM_SIZE
#define MAX_KEYSIZE_SIZE 1
#define MAX_KEYSIZE_OFFSET MAGIC_NUM_SIZE + CURR_NODE_NUM_SIZE
#define CURR_ROOT_PID_SIZE 4
#define CURR_ROOT_PID_OFFSET MAGIC_NUM_SIZE + CURR_NODE_NUM_SIZE + MAX_KEYSIZE_SIZE

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
