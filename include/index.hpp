#pragma once

#include "bpm.h"
#include <cstdint>
#include <cstring>
#include <string>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

#include "./shared.h"
#include "disk_manager.h"

// Type used for walking back up the tree after reaching leaf node,
// containing parent pointer and index in pointer array
// index is -1 if the node was pointed to by the rightmost_ptr of the parent
#define BREADCRUMB_TYPE std::pair<BTreePage *, int>
#define LEAF_RECORDS(records) std::get<leaf_records>(records)

namespace somedb {

struct BTreeKey {
    u8 *data;
    u16 length;
};

using leaf_records = std::vector<RID>;

struct BTreePage {
    bool is_leaf;
    uint16_t max_size; // max number of elements in a node (aka node's degree-1)
    uint16_t level;    // starting from 0
    std::vector<BTreeKey> keys;

    // Tracking siblings for sequential scan across leaf nodes
    BTreePage *previous;
    BTreePage *next;

    // Depending on the node type, store either
    // record ids of tuples (leaf) or pointers to descendants (internal)
    std::variant<leaf_records, std::vector<BTreePage *>> records, children;

    BTreePage(bool is_leaf, u16 max_size);
};

struct BTree {
    std::string index_name;
    BTreePage *root;
    BufferPoolManager *bpm; // layer for fetching pages from memory into tree pages (aka tree nodes)

    BTree(std::string index_name, BufferPoolManager *bpm, uint16_t max_size = 8, BTreePage *root = nullptr);
    //~BPlusTree(); // TODO: DEALLOCATE TREE MEMORY

    bool getValues(const BTreeKey &key, std::vector<RID> &result);
    bool insert(const BTreeKey &key, const RID &val);

  private:
    BTreePage *findLeaf(const BTreeKey &key);
    void insertIntoLeaf(BTreePage *node, const BTreeKey &key, const RID &val);

    // If first key is bigger return >0, if second is bigger return <0, if equal 0
    inline static int cmpKeys(const u8 *key1, const u8 *key2, u16 len1, u16 len2) {
        u16 len = std::min(len1, len2);
        return memcmp(key1, key2, len);
    }
};

using internal_pointers = std::vector<BTreePage *>;

} // namespace somedb
