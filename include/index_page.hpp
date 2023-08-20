/*
 * On disk, following the metadata page, there is a list of b+tree pages
 *
 * Each b+tree page is layed out in the slotted page format (like in heap files), with the page header including:
 *  (16 bit uint) page offset to the beginning of available space
 *  (16 bit uint) page offset to the end of available space
 *  (32 bit uint) page id of previous (sibling) page
 *  (32 bit uint) page id of next (sibling) page
 *  (32 bit uint) page id of rightmost pointer (0 if none (only if page is leaf or there is one value in internal page))
 *  (8 bit uint) special page header flags
 *  (8 bit uint) is_leaf boolean
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

#include "./serialize.h"
#include "./shared.h"
#include "disk_manager.h"

#define TREE_NODE_FUNC_TYPE                                                                                            \
    template <typename VAL_T>                                                                                          \
    requires(std::same_as<VAL_T, RID> || std::same_as<VAL_T, u32>)

/* Extracting node values */
#define LEAF_RECORDS(records) std::get<std::vector<RID>>(records)
#define INTERNAL_CHILDREN(children) std::get<std::vector<u32>>(children)

namespace somedb {

using leaf_records = std::vector<RID>;
using internal_pointers = std::vector<u32>;

struct BTreePage {
    bool is_leaf;
    std::vector<BTreeKey> keys;
    u8 flags;

    // only storing file offsets of connected nodes so it's not necessary to load
    // them from disk before referencing them
    page_id_t rightmost_ptr;
    page_id_t next;

    /* Depending on node type, store record ids of tuples (leaf) or pointers to
     * descendants (internal). */
    std::variant<std::vector<RID>, std::vector<u32>> values;
    //--------------------------------------------------------------------------------------------------------------------------------
    /* Deserialize given page data into a BTreePage instance */
    BTreePage(u8 data[PAGE_SIZE]);

    BTreePage(bool is_leaf);
    //--------------------------------------------------------------------------------------------------------------------------------
    u8 *serialize() const;

    /* From page bytes, determine if the page is of leaf type*/
    inline bool get_is_leaf(u8 data[PAGE_SIZE]) { return *(data + IS_LEAF_OFFSET) == true; }

    /* If first key is bigger return >0, if second is bigger return <0, if equal
     * return 0 */
    inline static int cmpKeys(const u8 *key1, const u8 *key2, u16 len1, u16 len2) {
        u16 len = std::min(len1, len2);
        return memcmp(key1, key2, len);
    }

    /* Get the second half of the vector.
     * Include middle vector element at the beginning if INCLUDE_MID is true, leave out otherwise
     */
    template <typename T>
    inline static std::vector<T> vec_second_half(std::vector<T> full_vec, bool include_mid = true) {
        const u8 includer = include_mid ? 0 : 1;
        return {full_vec.begin() + (full_vec.size() / 2) + includer, full_vec.end()};
    }

    /* Get the first half of the vector
     * Include middle vector element at the end if INCLUDE_MID is true, leave out otherwise
     */
    template <typename T>
    inline static std::vector<T> vec_first_half(std::vector<T> full_vec, bool include_mid = false) {
        const u8 includer = include_mid ? 1 : 0;
        return {full_vec.begin(), full_vec.begin() + (full_vec.size() / 2) + includer};
    }

    /*
     * Inserts provided key and value into node (leaf or internal).
     * Key/value pairs inside a node remain sorted after the insertion.
     */
    TREE_NODE_FUNC_TYPE void insertIntoNode(const BTreeKey &key, VAL_T val);

    //--------------------------------------------------------------------------------------------------------------------------------
};

} // namespace somedb
