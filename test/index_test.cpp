#include "../include/disk/bpm.h"
#include "../include/disk/heapfile.h"
#include "../include/index/btree_index.hpp"
#include "../include/index/index_page.hpp"
#include "../include/utils/serialize.h"
#include <cstring>
#include <gtest/gtest.h>
#include <string>

namespace somedb {

class IndexTestFixture : public testing::Test {
  protected:
    std::string table_name = "bplus_test_table";
    std::string index_name = "bplus_test_table_idx";
    BufferPoolManager *bpm;
    DiskManager *disk_mgr;

    const u16 tree_max_size = 4;

    size_t keys_length;
    std::vector<BTreeKey> keys;
    std::vector<RID> vals;
    std::string keys_data[30] = {"A", "B", "D", "E", "C", "F", "G", "H", "I", "J",
                                 "K", "L", "M", "N", "O", "P", "Q", "R", "S"};
    std::unordered_map<std::string, int> key_to_idx = {
        {"A", 0},  {"B", 1},  {"C", 4},  {"D", 2},  {"E", 3},  {"F", 5},  {"G", 6},  {"H", 7},  {"I", 8},  {"J", 9},
        {"K", 10}, {"L", 11}, {"M", 12}, {"N", 13}, {"O", 14}, {"P", 15}, {"Q", 16}, {"R", 17}, {"S", 18},
    };

    void SetUp() override {
        // Table setup
        char cname1[5] = "name";
        char cname2[5] = "age";
        Column cols[2] = {{.name_len = (u8)strlen(cname1), .name = cname1, .type = STRING},
                          {.name_len = (u8)strlen(cname2), .name = cname2, .type = INTEGER}};
        create_table(table_name.data(), cols, (sizeof(cols) / sizeof(Column)));

        // Index setup (file, cache etc.)
        disk_mgr = create_btree_index(index_name.data(), tree_max_size);
        disk_mgr->page_type = BTREE_INDEX_PAGE;
        bpm = new_bpm(100, disk_mgr);

        // Data setup
        keys_length = sizeof(keys_data) / sizeof(keys_data[0]);
        for (u16 i = 0; i < keys_length; i++) {
            auto key = new BTreeKey();
            key->length = static_cast<u8>(keys_data[i].length());
            key->data = reinterpret_cast<u8 *>(keys_data[i].data());
            keys.push_back(*key);

            auto rec = new RID();
            rec->pid = i + 1;
            rec->slot_num = i + 2;
            vals.push_back(*rec);
        }
    }

    void TearDown() override {
        remove_table(table_name.data());
        remove_table(index_name.data());
    }

    TREE_NODE_FUNC_TYPE void TestEqualNode(BTreePage node_actual, std::vector<std::string> keys_idx_exp) {
        std::vector<BTreeKey> keys_exp;
        for (auto key : keys_idx_exp)
            keys_exp.emplace_back(keys.at(key_to_idx.at(key)));

        EXPECT_EQ(node_actual.keys, keys_exp);

        // Test values only for leaf nodes, since internal nodes' values are implicitly tested by testing their children
        if constexpr (std::same_as<VAL_T, RID>) {
            std::vector<RID> leafval_exp;
            for (auto key : keys_idx_exp) {
                leafval_exp.emplace_back(vals.at(key_to_idx.at(key)));
            }
            EXPECT_EQ(LEAF_RECORDS(node_actual.values), leafval_exp);
        }
    }

    // Insert array of elements in the tree
    void Insert(BTree &tree, std::vector<std::string> keys_to_ins) {
        for (auto key : keys_to_ins) {
            auto idx = key_to_idx.at(key);
            tree.insert(keys[idx], vals[idx]);
        }
    }

    // Fetches the btree page/node of provided pid from the buffer pool and forms a BTreePage object out of it
    BTreePage GetNode(page_id_t pid, BTree tree) { return BTreePage(fetch_bpm_page(pid, tree.bpm)->data); };
};

TEST_F(IndexTestFixture, CreateIndex_SerializeAndDeserializeTree) {
    // check the tree metadata page (created with create_btree_index in the setup)
    BTree tree(bpm);
    tree.deserialize();
    EXPECT_EQ(tree.magic_num, BTREE_INDEX);
    EXPECT_EQ(tree.node_count, 0);
    EXPECT_EQ(tree.max_size, tree_max_size);
}

TEST_F(IndexTestFixture, CreateIndex_SerializeAndDeserializeNode) {
    BTreePage page(true);
    page.rightmost_ptr = 0;
    page.next = 256;
    RID rids[] = {{.pid = 1, .slot_num = 3}, {.pid = 2, .slot_num = 4}, {.pid = 3, .slot_num = 5}};
    std::string keys_data[] = {"key", "key1", "key82"};

    for (const RID &rid : rids)
        LEAF_RECORDS(page.values).push_back(rid);

    for (std::string &data : keys_data) {
        BTreeKey key = {reinterpret_cast<u8 *>(data.data()), static_cast<u8>(data.length())};
        page.keys.push_back(key);
    }
    u8 num_pairs = page.keys.size();

    // write to disk
    auto bpm_page = allocate_new_page(bpm, BTREE_INDEX_PAGE);
    frame_id_t *frame_id =
        static_cast<frame_id_t *>(hash_find(std::to_string(bpm_page->id).data(), bpm->page_table)->data);
    write_to_frame(*frame_id, page.serialize(), bpm);
    flush_page(bpm_page->id, bpm);

    // check if its correctly written to disk
    u8 *serialized = read_page(bpm_page->id, disk_mgr);
    EXPECT_EQ(decode_uint16(serialized + AVAILABLE_SPACE_START_OFFSET),
              INDEX_PAGE_HEADER_SIZE + (TREE_KV_PTR_SIZE * num_pairs));
    EXPECT_EQ(decode_uint16(serialized + AVAILABLE_SPACE_END_SIZE), PAGE_SIZE - (24 + 3 + 12));
    EXPECT_EQ(decode_uint32(serialized + NEXT_PID_OFFSET), page.next);
    EXPECT_EQ(decode_uint32(serialized + RIGHTMOST_PID_OFFSET), page.rightmost_ptr);
    EXPECT_EQ(*(serialized + TREE_FLAGS_OFFSET), page.flags);

    // deserialize into BTreePage
    BTreePage page_deserialized(serialized);
    EXPECT_EQ(page_deserialized.rightmost_ptr, page.rightmost_ptr);
    EXPECT_EQ(page_deserialized.is_leaf, page.is_leaf);
    EXPECT_EQ(page_deserialized.next, page.next);
    for (u8 i = 0; i < page_deserialized.keys.size(); i++) {
        // compare keys as they were added to keys read from end of page, same for values
        EXPECT_EQ(page_deserialized.keys.at(i).length, page.keys.at(i).length);
        EXPECT_EQ(*page_deserialized.keys.at(i).data, *page.keys.at(i).data);

        auto read_record = LEAF_RECORDS(page_deserialized.values).at(i);
        auto actual_record = LEAF_RECORDS(page.values).at(i);
        EXPECT_EQ(read_record.pid, actual_record.pid);
        EXPECT_EQ(read_record.slot_num, actual_record.slot_num);
    }
}

TEST_F(IndexTestFixture, InsertTest_RootWithEnoughSpace) {
    // check the tree metadata page (created with create_btree_index)
    u8 *metadata = read_page(BTREE_METADATA_PAGE_ID, bpm->disk_manager);
    EXPECT_EQ(decode_uint32(metadata + MAGIC_NUMBER_OFFSET), BTREE_INDEX);
    BTree tree(bpm);
    tree.deserialize();

    // insert kv pair
    std::string data1 = "1";
    BTreeKey key1 = {reinterpret_cast<u8 *>(data1.data()), static_cast<u8>(data1.length())};
    RID val1 = {.pid = 4, .slot_num = 5}; // doesn't need to be actual rid
    page_id_t leaf_pid = tree.insert(key1, val1);

    auto bpm_page = fetch_bpm_page(leaf_pid, tree.bpm);
    BTreePage leaf(bpm_page->data);

    EXPECT_EQ(leaf.is_leaf, true);
    EXPECT_EQ((char *)leaf.keys.at(0).data == data1, true);
    EXPECT_EQ(LEAF_RECORDS(leaf.values).at(0).pid, val1.pid);

    // insert another and check sorting
    std::string data2 = "0";
    BTreeKey key2 = {reinterpret_cast<u8 *>(data2.data()), static_cast<u8>(data2.length())};
    leaf_pid = tree.insert(key2, val1);
    bpm_page = fetch_bpm_page(leaf_pid, tree.bpm);
    leaf = BTreePage(bpm_page->data);
    EXPECT_EQ((char *)leaf.keys.at(0).data == data2, true);
    EXPECT_EQ((char *)leaf.keys.at(1).data == data1, true);
    EXPECT_EQ(LEAF_RECORDS(leaf.values).at(0).pid, val1.pid);
    EXPECT_EQ(LEAF_RECORDS(leaf.values).at(1).pid, val1.pid);
}

TEST_F(IndexTestFixture, InsertTest_Splits) {
    BTree tree(bpm);
    tree.deserialize();

    // Test simple insert
    auto leaf_pid1 = tree.insert(keys[0], vals[0]);
    Insert(tree, {"B", "D"});
    auto leaf_pid = tree.insert(keys[3], vals[3]);
    EXPECT_EQ(leaf_pid, leaf_pid1); // all kv pairs are in same node

    auto curr_root = GetNode(leaf_pid, tree);

    // Test duplicate
    auto pre_size = curr_root.keys.size();
    bool ok = tree.insert(keys[0], vals[0]);
    EXPECT_EQ(ok, false);
    EXPECT_EQ(curr_root.keys.size(), pre_size);

    // Test root node split
    Insert(tree, {"C"});
    curr_root = GetNode(tree.root_pid, tree);
    auto old_root = GetNode(INTERNAL_CHILDREN(curr_root.values).at(0), tree);
    auto new_child = GetNode(curr_root.rightmost_ptr, tree);
    TestEqualNode<u32>(curr_root, {"C"});
    TestEqualNode<RID>(old_root, {"A", "B"});
    TestEqualNode<RID>(new_child, {"C", "D", "E"});

    // Test leaf node split
    Insert(tree, {"F", "G"});
    curr_root = GetNode(tree.root_pid, tree);
    TestEqualNode<u32>(GetNode(tree.root_pid, tree), {"C", "E"});
    TestEqualNode<RID>(GetNode(INTERNAL_CHILDREN(curr_root.values).at(0), tree), {"A", "B"});
    TestEqualNode<RID>(GetNode(INTERNAL_CHILDREN(curr_root.values).at(1), tree), {"C", "D"});
    TestEqualNode<RID>(GetNode(INTERNAL_CHILDREN(curr_root.values).at(1), tree), {"C", "D"});
    TestEqualNode<RID>(GetNode(curr_root.rightmost_ptr, tree), {"E", "F", "G"});

    // Add elements until internal node split
    Insert(tree, {"H", "I", "J", "K", "L", "M"});

    // Test root and second level
    curr_root = GetNode(tree.root_pid, tree);
    EXPECT_EQ(curr_root.keys.size(), 1);
    auto left_internal = GetNode(INTERNAL_CHILDREN(curr_root.values).at(0), tree);
    auto right_internal = GetNode(curr_root.rightmost_ptr, tree);
    TestEqualNode<u32>(left_internal, {"C", "E"});
    TestEqualNode<u32>(right_internal, {"I", "K"});

    // Test third level (leaf nodes)
    TestEqualNode<RID>(GetNode(INTERNAL_CHILDREN(left_internal.values).at(0), tree), {"A", "B"});
    TestEqualNode<RID>(GetNode(INTERNAL_CHILDREN(left_internal.values).at(1), tree), {"C", "D"});
    TestEqualNode<RID>(GetNode(left_internal.rightmost_ptr, tree), {"E", "F"});
    //--------
    TestEqualNode<RID>(GetNode(INTERNAL_CHILDREN(right_internal.values).at(0), tree), {"G", "H"});
    TestEqualNode<RID>(GetNode(INTERNAL_CHILDREN(right_internal.values).at(1), tree), {"I", "J"});
    TestEqualNode<RID>(GetNode(right_internal.rightmost_ptr, tree), {"K", "L", "M"});
}

TEST_F(IndexTestFixture, RemoveTest_NoMerges) {
    BTree tree(bpm);
    tree.deserialize();

    // Insert two elements
    Insert(tree, {"A", "B"});
    TestEqualNode<RID>(GetNode(tree.root_pid, tree), {"A", "B"});

    // Test removing one and having one left
    tree.remove(keys[0]);
    TestEqualNode<RID>(GetNode(tree.root_pid, tree), {"B"});
}

TEST_F(IndexTestFixture, RemoveTest_Merges) {
    BTree tree(bpm);
    tree.deserialize();

    // Insert elements until there are 3 levels (1 root, 1 internal, 1 leaf level)
    for (int i = 0; i <= 12; i++) { // keys_data[12] = "M"
        tree.insert(keys[i], vals[i]);
    }

    // Remove right most element in leaf node "M", which should trigger merge of last two leaf nodes (since both now
    // have 2 elements), Which should in turn trigger a merge on the level above (since the two internal nodes combined
    // now have 3 elements), Which should in turn trigger a merge on the root level since there is now only one internal
    // node left. Now we should have a tree of 1 less level
    tree.remove(keys[12]);
    BTreePage curr_root = GetNode(tree.root_pid, tree);
    BTreePage root_rightmost(fetch_bpm_page(curr_root.rightmost_ptr, tree.bpm)->data);
    TestEqualNode<RID>(GetNode(INTERNAL_CHILDREN(curr_root.values).at(0), tree), {"A", "B"});
    TestEqualNode<RID>(GetNode(INTERNAL_CHILDREN(curr_root.values).at(1), tree), {"C", "D"});
    TestEqualNode<RID>(GetNode(INTERNAL_CHILDREN(curr_root.values).at(2), tree), {"E", "F"});
    TestEqualNode<RID>(GetNode(INTERNAL_CHILDREN(curr_root.values).at(3), tree), {"G", "H"});
    TestEqualNode<RID>(GetNode(curr_root.rightmost_ptr, tree), {"I", "J", "K", "L"});

    // Remove "K" and "L", leaving last leaf with two elements.
    // Since its previous sibling ("G" "H") has two elements at this point, they should merge.
    // After removal, leaf node level should look like this:
    // (A,B) -> (C,D) -> (E,F) -> (G,H,I,J)
    tree.remove(keys[10]);
    tree.remove(keys[11]);
    curr_root = GetNode(tree.root_pid, tree);
    EXPECT_EQ(curr_root.keys.size(), 3);
    // just check the rightmost child at this stage since the rest are not changed
    TestEqualNode<RID>(GetNode(curr_root.rightmost_ptr, tree), {"G", "H", "I", "J"});

    // Remove "I" and "J", this should trigger another leaf merge
    // Now leaf level should look like:
    // (A,B) -> (C,D) -> (E,F,G,H)
    tree.remove(keys[8]);
    tree.remove(keys[9]);
    curr_root = GetNode(tree.root_pid, tree);
    EXPECT_EQ(curr_root.keys.size(), 2);
    TestEqualNode<RID>(GetNode(curr_root.rightmost_ptr, tree), {"E", "F", "G", "H"});

    // Remove "A", "B", "D"
    // Now there should be two leaf nodes since last one is full (no merge with first)
    // (C) -> (E,F,G,H)
    tree.remove(keys[0]);
    tree.remove(keys[1]);
    tree.remove(keys[2]);
    curr_root = GetNode(tree.root_pid, tree);
    EXPECT_EQ(curr_root.keys.size(), 1);
    TestEqualNode<RID>(GetNode(INTERNAL_CHILDREN(curr_root.values).at(0), tree), {"C"});
    TestEqualNode<RID>(GetNode(curr_root.rightmost_ptr, tree), {"E", "F", "G", "H"});

    // Remove "H"
    // This should trigger a merge, leaving the tree with a single leaf node that becomes root
    tree.remove(keys[7]);
    curr_root = GetNode(tree.root_pid, tree);
    TestEqualNode<RID>(curr_root, {"C", "E", "F", "G"});

    // Remove the remaining elements
    tree.remove(keys[6]);
    tree.remove(keys[5]);
    tree.remove(keys[3]);
    tree.remove(keys[4]);
    curr_root = GetNode(tree.root_pid, tree);
    EXPECT_EQ(curr_root.keys.size(), 0);
    EXPECT_EQ(LEAF_RECORDS(curr_root.values).size(), 0);
}

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
} // namespace somedb
