#include "../include/bpm.h"
#include "../include/btree_index.hpp"
#include "../include/heapfile.h"
#include "../include/index_page.hpp"
#include "../include/serialize.h"
#include <cstring>
#include <gtest/gtest.h>
#include <string>

namespace somedb {

class IndexTestFixture : public testing::Test {
  protected:
    const char table_name[20] = "bplus_test_table";
    const char index_name[21] = "bplus_test_table_idx";
    BufferPoolManager *bpm;
    DiskManager *disk_mgr;

    const u16 tree_max_size = 4;

    void SetUp() override {
        char cname1[5] = "name";
        char cname2[5] = "age";
        Column cols[2] = {{.name_len = (u8)strlen(cname1), .name = cname1, .type = STRING},
                          {.name_len = (u8)strlen(cname2), .name = cname2, .type = UINT16}};
        create_table(table_name, cols, (sizeof(cols) / sizeof(Column)));

        disk_mgr = create_btree_index(index_name, tree_max_size);
        disk_mgr->page_type = BTREE_INDEX_PAGE;
        bpm = new_bpm(100, disk_mgr);
    }

    void TearDown() override {
        remove_table(table_name);
        remove_table(index_name);
    }
};

TEST_F(IndexTestFixture, CreateIndex_SerializeAndDeserializeTree) {
    // check the tree metadata page (created with create_btree_index in the setup)
    auto tree = new BTree(bpm);
    tree->deserialize();
    EXPECT_EQ(tree->magic_num, BTREE_INDEX);
    EXPECT_EQ(tree->node_count, 0);
    EXPECT_EQ(tree->max_size, tree_max_size);
}

TEST_F(IndexTestFixture, CreateIndex_SerializeAndDeserializeNode) {
    auto page = new BTreePage(true);
    page->rightmost_ptr = 0;
    page->next = 256;
    page->previous = 64;
    RID rids[] = {{.pid = 1, .slot_num = 3}, {.pid = 2, .slot_num = 4}, {.pid = 3, .slot_num = 5}};
    std::string keys_data[] = {"key", "key1", "key82"};

    for (const RID &rid : rids)
        LEAF_RECORDS(page->values).push_back(rid);

    for (std::string &data : keys_data) {
        BTreeKey key = {reinterpret_cast<u8 *>(data.data()), static_cast<u8>(data.length())};
        page->keys.push_back(key);
    }
    u8 num_pairs = page->keys.size();

    // write to disk
    auto bpm_page = allocate_new_page(bpm, BTREE_INDEX_PAGE);
    frame_id_t *frame_id =
        static_cast<frame_id_t *>(hash_find(std::to_string(bpm_page->id).data(), bpm->page_table)->data);
    write_to_frame(*frame_id, page->serialize(), bpm);
    flush_page(bpm_page->id, bpm);

    // check if its correctly written to disk
    u8 *serialized = read_page(bpm_page->id, disk_mgr);
    EXPECT_EQ(decode_uint16(serialized + AVAILABLE_SPACE_START_OFFSET),
              INDEX_PAGE_HEADER_SIZE + (TREE_KV_PTR_SIZE * num_pairs));
    EXPECT_EQ(decode_uint16(serialized + AVAILABLE_SPACE_END_SIZE), PAGE_SIZE - (24 + 3 + 12));
    EXPECT_EQ(decode_uint32(serialized + PREVIOUS_PID_OFFSET), page->previous);
    EXPECT_EQ(decode_uint32(serialized + NEXT_PID_OFFSET), page->next);
    EXPECT_EQ(decode_uint32(serialized + RIGHTMOST_PID_OFFSET), page->rightmost_ptr);
    EXPECT_EQ(decode_uint16(serialized + TREE_LEVEL_OFFSET), page->level);
    EXPECT_EQ(*(serialized + TREE_FLAGS_OFFSET), page->flags);

    // deserialize into BTreePage
    auto page_deserialized = new BTreePage(serialized);
    EXPECT_EQ(page_deserialized->rightmost_ptr, page->rightmost_ptr);
    EXPECT_EQ(page_deserialized->is_leaf, page->is_leaf);
    EXPECT_EQ(page_deserialized->previous, page->previous);
    EXPECT_EQ(page_deserialized->next, page->next);
    for (u8 i = 0; i < page_deserialized->keys.size(); i++) {
        // compare keys as they were added to keys read from end of page, same for values
        EXPECT_EQ(page_deserialized->keys.at(i).length, page->keys.at(i).length);
        EXPECT_EQ(*page_deserialized->keys.at(i).data, *page->keys.at(i).data);

        auto read_record = LEAF_RECORDS(page_deserialized->values).at(i);
        auto actual_record = LEAF_RECORDS(page->values).at(i);
        EXPECT_EQ(read_record.pid, actual_record.pid);
        EXPECT_EQ(read_record.slot_num, actual_record.slot_num);
    }
}

TEST_F(IndexTestFixture, InsertTest_RootWithEnoughSpace) {
    // check the tree metadata page (created with create_btree_index)
    u8 *metadata = read_page(BTREE_METADATA_PAGE_ID, bpm->disk_manager);
    EXPECT_EQ(decode_uint32(metadata + MAGIC_NUMBER_OFFSET), BTREE_INDEX);
    auto tree = new BTree(bpm);
    tree->deserialize();

    // insert kv pair
    std::string data1 = "1";
    BTreeKey key1 = {reinterpret_cast<u8 *>(data1.data()), static_cast<u8>(data1.length())};
    RID val1 = {.pid = 4, .slot_num = 5}; // doesn't need to be actual rid
    page_id_t leaf_pid = tree->insert(key1, val1);

    auto bpm_page = fetch_bpm_page(leaf_pid, tree->bpm);
    auto leaf = new BTreePage(bpm_page->data);

    EXPECT_EQ(leaf->is_leaf, true);
    EXPECT_EQ((char *)leaf->keys.at(0).data == data1, true);
    EXPECT_EQ(LEAF_RECORDS(leaf->values).at(0).pid, val1.pid);

    // insert another and check sorting
    std::string data2 = "0";
    BTreeKey key2 = {reinterpret_cast<u8 *>(data2.data()), static_cast<u8>(data2.length())};
    leaf_pid = tree->insert(key2, val1);
    bpm_page = fetch_bpm_page(leaf_pid, tree->bpm);
    leaf = new BTreePage(bpm_page->data);
    EXPECT_EQ((char *)leaf->keys.at(0).data == data2, true);
    EXPECT_EQ((char *)leaf->keys.at(1).data == data1, true);
    EXPECT_EQ(LEAF_RECORDS(leaf->values).at(0).pid, val1.pid);
    EXPECT_EQ(LEAF_RECORDS(leaf->values).at(1).pid, val1.pid);
}

TEST_F(IndexTestFixture, InsertTest_FullRoot) {
    auto tree = new BTree(bpm);
    tree->deserialize();
    RID vals[] = {{.pid = 1, .slot_num = 2},
                  {.pid = 3, .slot_num = 4},
                  {.pid = 5, .slot_num = 6},
                  {.pid = 7, .slot_num = 8},
                  {.pid = 9, .slot_num = 10}};
    std::string keys_data[] = {"A", "B", "D", "E", "C"};
    BTreeKey keys[] = {
        {reinterpret_cast<u8 *>(keys_data[0].data()), static_cast<u8>(keys_data[0].length())},
        {reinterpret_cast<u8 *>(keys_data[1].data()), static_cast<u8>(keys_data[1].length())},
        {reinterpret_cast<u8 *>(keys_data[2].data()), static_cast<u8>(keys_data[2].length())},
        {reinterpret_cast<u8 *>(keys_data[3].data()), static_cast<u8>(keys_data[3].length())},
        {reinterpret_cast<u8 *>(keys_data[4].data()), static_cast<u8>(keys_data[4].length())},
    };

    auto leaf_pid1 = tree->insert(keys[0], vals[0]);
    tree->insert(keys[1], vals[1]);
    tree->insert(keys[2], vals[2]);
    auto leaf_pid = tree->insert(keys[3], vals[3]);
    EXPECT_EQ(leaf_pid, leaf_pid1); // all kv pairs are in same node

    auto curr_root = new BTreePage(fetch_bpm_page(leaf_pid, tree->bpm));

    // Test duplicate
    auto pre_size = curr_root->keys.size();
    bool ok = tree->insert(keys[0], vals[0]);
    EXPECT_EQ(ok, false);
    EXPECT_EQ(curr_root->keys.size(), pre_size);

    // Test node split
    tree->insert(keys[4], vals[4]);
    curr_root = new BTreePage(fetch_bpm_page(tree->root_pid, tree->bpm)->data);
    auto old_root = new BTreePage(fetch_bpm_page(INTERNAL_CHILDREN(curr_root->values).at(0), tree->bpm)->data);
    auto new_child = new BTreePage(fetch_bpm_page(curr_root->rightmost_ptr, tree->bpm)->data);
    EXPECT_EQ((char *)curr_root->keys.at(0).data == keys_data[4], true); // "C" in middle
    EXPECT_EQ(curr_root->keys.size(), 1);
    std::vector<BTreeKey> left_node_keys = {keys[0], keys[1]};           // A B
    std::vector<BTreeKey> right_node_keys = {keys[4], keys[2], keys[3]}; // C D E
    EXPECT_EQ(old_root->keys == left_node_keys, true);
    EXPECT_EQ(new_child->keys == right_node_keys, true);
}

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
} // namespace somedb
