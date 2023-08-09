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
#pragma once
#include "bpm.h"
#include <stack>
#include <variant>
#include <vector>

#include "./shared.h"
#include "disk_manager.h"

#define TREE_NODE_FUNC_TYPE                                                                                            \
    template <typename VAL_T>                                                                                          \
    requires(std::same_as<VAL_T, RID> || std::same_as<VAL_T, u32>)

/* Extracting node values */
#define LEAF_RECORDS(records) std::get<std::vector<RID>>(records)
#define INTERNAL_CHILDREN(children) std::get<std::vector<u32>>(children)

/* Serialization sizes and offsets */
#define TREE_KV_PTR_SIZE 4
#define AVAILABLE_SPACE_START_SIZE 2
#define AVAILABLE_SPACE_START_OFFSET 0
#define AVAILABLE_SPACE_END_SIZE 2
#define AVAILABLE_SPACE_END_OFFSET AVAILABLE_SPACE_START_OFFSET + AVAILABLE_SPACE_START_SIZE
#define PREVIOUS_PID_SIZE 4
#define PREVIOUS_PID_OFFSET AVAILABLE_SPACE_END_OFFSET + AVAILABLE_SPACE_END_SIZE
#define NEXT_PID_SIZE 4
#define NEXT_PID_OFFSET PREVIOUS_PID_OFFSET + PREVIOUS_PID_SIZE
#define RIGHTMOST_PID_SIZE 4
#define RIGHTMOST_PID_OFFSET NEXT_PID_OFFSET + NEXT_PID_SIZE
#define TREE_LEVEL_SIZE 2
#define TREE_LEVEL_OFFSET RIGHTMOST_PID_OFFSET + RIGHTMOST_PID_SIZE
#define TREE_FLAGS_SIZE 1
#define TREE_FLAGS_OFFSET TREE_LEVEL_OFFSET + TREE_LEVEL_SIZE
#define INDEX_PAGE_HEADER_SIZE 17
#define IS_LEAF_SIZE 1
#define IS_LEAF_OFFSET 16

namespace somedb {

using leaf_records = std::vector<RID>;
using internal_pointers = std::vector<u32>;

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
    //--------------------------------------------------------------------------------------------------------------------------------
    u8 *serialize() const;

    /* From page bytes, determine if the page is of leaf type*/
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
     */
    TREE_NODE_FUNC_TYPE void insertIntoNode(const BTreeKey &key, VAL_T val);

    //--------------------------------------------------------------------------------------------------------------------------------
};

} // namespace somedb
