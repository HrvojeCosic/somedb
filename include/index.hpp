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

namespace somedb {

struct BTreeKey {
    u8 *data;
    u16 length;
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

    void insert(const BTreeKey &key, const RID &val);

  private:
    BTreePage *findLeaf(const BTreeKey &key, std::stack<BREADCRUMB_TYPE> &breadcrumbs);

    /*
     * Inserts provided key and value into node (leaf or internal).
     * Key/value pairs inside a node remain sorted after the insertion.
     * Provided VAL can be of two types (as seen in BTreePage), depending on if insertion is being done in leaf or
     * internal node
     */
    template <typename VAL_T>
    requires(std::same_as<VAL_T, RID> ||
             std::same_as<VAL_T, BTreePage *>) void insertIntoNode(BTreePage *node, const BTreeKey &key, VAL_T val);

    void splitNode(BTreePage *node, std::stack<BREADCRUMB_TYPE> breadcrumbs);

    /* If first key is bigger return >0, if second is bigger return <0, if equal 0 */
    inline static int cmpKeys(const u8 *key1, const u8 *key2, u16 len1, u16 len2) {
        u16 len = std::min(len1, len2);
        return memcmp(key1, key2, len);
    }
};

using internal_pointers = std::vector<BTreePage *>;

} // namespace somedb
