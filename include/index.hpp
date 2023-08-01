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
#define INTERNAL_CHILDREN(children) std::get<std::vector<BTreePage *>>(children)

/* Template of functions that work with nodes and support both node types */
#define TREE_NODE_FUNC_TYPE                                                                                            \
    template <typename VAL_T>                                                                                          \
    requires(std::same_as<VAL_T, RID> || std::same_as<VAL_T, BTreePage *>)

namespace somedb {

struct BTreeKey {
    u8 *data;
    u16 length;

    bool operator==(const BTreeKey &other) const {
        return (length == other.length) && (std::memcmp(data, other.data, length) == 0);
    }
};

struct BTreePage {
    bool is_leaf;
    uint16_t max_size; // max number of elements in a node (aka node's degree-1)
    uint16_t level;    // starting from 0
    std::vector<BTreeKey> keys;

    /* Tracking siblings for sequential scan across leaf nodes */
    BTreePage *previous;
    BTreePage *next;

    /*
     * Depending on the node type, store either
     *  record ids of tuples (leaf) or pointers to descendants (internal)
     */
    std::variant<std::vector<RID>, std::vector<BTreePage *>> records, children;

    BTreePage(bool is_leaf, u16 max_size);
};

struct BTree {
    std::string index_name;
    BTreePage *root;
    BufferPoolManager *bpm; // layer for fetching pages from memory into tree pages (aka tree nodes

    BTree(std::string index_name, BufferPoolManager *bpm, uint16_t max_size = 8, BTreePage *root = nullptr);
    //~BPlusTree(); // TODO: DEALLOCATE TREE MEMORY

    bool getValues(const BTreeKey &key, std::vector<RID> &result);

    /* Insert element into the tree and return true if successful, of false if provided key already exists */
    bool insert(const BTreeKey &key, const RID &val);

  private:
    BTreePage *findLeaf(const BTreeKey &key, std::stack<BREADCRUMB_TYPE> &breadcrumbs);

    void splitRootNode(BTreePage *node);

    /*
     * Inserts provided key and value into node (leaf or internal).
     * Key/value pairs inside a node remain sorted after the insertion.
     * Provided VAL can be of two types (as seen in BTreePage), depending on if insertion is being done in leaf or
     * internal node
     */
    TREE_NODE_FUNC_TYPE
    void insertIntoNode(BTreePage *node, const BTreeKey &key, VAL_T val);

    TREE_NODE_FUNC_TYPE
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
};

using internal_pointers = std::vector<BTreePage *>;

} // namespace somedb
