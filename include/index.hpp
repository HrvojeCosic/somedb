#pragma once

#include "bpm.h"
#include <concepts>
#include <cstdint>
#include <cstring>
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

/*
 * When creating a BTreePage class, we need to specify whether it's of internal or leaf type by passing RID or u16
 * (which are types of values for these node types, respectively
 */
#define TREE_NODE_TYPE                                                                                                 \
    template <typename VAL_T>                                                                                          \
    requires(std::same_as<VAL_T, RID> || std::same_as<VAL_T, u16>)

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

/* BTree struct is an in memory representation of a metadata page stored in an index file.
 *
 * The index file starts with the metadata page of the following format:
 * (32 bit uint) magic number, expected to equal BTREE_INDEX
 * (16 bit uint) current number of nodes, not including root
 * (8 bit uint) max number of keys in a node (max size/node's degree - 1)
 */
struct BTree {
    DiskManager *disk_manager;
    u32 magic_num;
    u16 node_count;
    u8 max_size;

    u8 *serialize() const;

    void deserialize(u8 data[PAGE_SIZE]);

    static page_id_t new_page();

    static DiskManager *create_index(const char *idx_name, const u8 max_keys);
};

/*
 * On disk, following the metadata page, there is a list of b+tree pages
 *
 * Each b+tree page is layed out in the slotted page format (like in heap files), with the page header including:
 *  (16 bit uint) page offset to the beginning of available space
 *  (16 bit uint) page offset to the end of available space
 *  (32 bit uint) file offset to the previous (sibling) page
 *  (32 bit uint) file offset to the next (sibling) page
 *  (16 bit uint) page offset to the rightmost pointer (set to 0 for leaf nodes, which is a way of identifying leafs)
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
TREE_NODE_TYPE struct BTreePage {
    bool is_leaf;
    u16 level; // starting from 0
    std::vector<BTreeKey> keys;
    u8 flags;

    /*
     * Tracking siblings for sequential scan across leaf nodes.
     * only storing their file offsets so it's not necessary to load them from disk before referencing them
     */
    u32 previous_off;
    u32 next_off;

    u32 rightmost_ptr_off;

    /* Depending on the node type, store either record ids of tuples (leaf) or pointers (file offsets) to descendants
     * (internal). */
    std::variant<std::vector<RID>, std::vector<u32>> records, children;
    //--------------------------------------------------------------------------------------------------------------------------------

    u8 *serialize() const;

    void deserialize(u8 data[PAGE_SIZE]);

    bool getValues(const BTreeKey &key, std::vector<RID> &result);

    /* Insert element into the tree and return true if successful, of false if provided key already exists */
    bool insert(const BTreeKey &key, const RID &val);

    //--------------------------------------------------------------------------------------------------------------------------------
  private:
    BTreePage *findLeaf(const BTreeKey &key, std::stack<BREADCRUMB_TYPE> &breadcrumbs);

    void splitRootNode(BTreePage *node);

    /*
     * Inserts provided key and value into node (leaf or internal).
     * Key/value pairs inside a node remain sorted after the insertion.
     * Provided VAL can be of two types (as seen in BTreePage), depending on if insertion is being done in leaf or
     * internal node
     */
    void insertIntoNode(BTreePage *node, const BTreeKey &key, VAL_T val);

    inline auto get_node_vals(BTreePage *node);

    /* If first key is bigger return >0, if second is bigger return <0, if equal 0 */
    inline static int cmpKeys(const u8 *key1, const u8 *key2, u16 len1, u16 len2) {
        u16 len = std::min(len1, len2);
        return memcmp(key1, key2, len);
    }

    /* Get the second half of the vector (useful for things like node splits) */
    template <typename T> inline static std::vector<T> vec_second_half(std::vector<T> full_vec) {
        return {full_vec.begin() + (full_vec.size() / 2), full_vec.end()};
    }

    /* Get the first half of the vector */
    template <typename T> inline static std::vector<T> vec_first_half(std::vector<T> full_vec) {
        return {full_vec.begin(), full_vec.begin() + full_vec.size() / 2};
    }

    //--------------------------------------------------------------------------------------------------------------------------------
};

} // namespace somedb
