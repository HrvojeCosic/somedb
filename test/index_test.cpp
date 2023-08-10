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
    ASSERT_EQ(tree->magic_num, BTREE_INDEX);
    ASSERT_EQ(tree->node_count, 0);
    ASSERT_EQ(tree->max_size, tree_max_size);
}

TEST_F(IndexTestFixture, CreateIndex_SerializeAndDeserializeNode) {
    auto page = new BTreePage(true);
    page->rightmost_ptr = 0;
    page->next = 256;
    page->previous = 64;
    RID rid1 = {.pid = 1, .slot_num = 3};
    RID rid2 = {.pid = 2, .slot_num = 4};
    RID rid3 = {.pid = 3, .slot_num = 5};
    LEAF_RECORDS(page->records).push_back(rid1);
    LEAF_RECORDS(page->records).push_back(rid2);
    LEAF_RECORDS(page->records).push_back(rid3);
    std::string data1 = "key";
    std::string data2 = "key1";
    std::string data3 = "key82";
    BTreeKey key1 = {reinterpret_cast<u8 *>(data1.data()), static_cast<u8>(data1.length())};
    BTreeKey key2 = {reinterpret_cast<u8 *>(data2.data()), static_cast<u8>(data2.length())};
    BTreeKey key3{reinterpret_cast<u8 *>(data3.data()), static_cast<u8>(data3.length())};
    u8 num_pairs = 3;
    page->keys = {key1, key2, key3};

    // write to disk
    auto bpm_page = allocate_new_page(bpm, BTREE_INDEX_PAGE);
    frame_id_t *frame_id =
        static_cast<frame_id_t *>(hash_find(std::to_string(bpm_page->id).data(), bpm->page_table)->data);
    write_to_frame(*frame_id, page->serialize(), bpm);
    flush_page(bpm_page->id, bpm);

    // check if its correctly written to disk
    u8 *serialized = read_page(bpm_page->id, disk_mgr);
    ASSERT_EQ(decode_uint16(serialized + AVAILABLE_SPACE_START_OFFSET),
              INDEX_PAGE_HEADER_SIZE + (TREE_KV_PTR_SIZE * num_pairs));
    ASSERT_EQ(decode_uint16(serialized + AVAILABLE_SPACE_END_SIZE), PAGE_SIZE - (24 + 3 + 12));
    ASSERT_EQ(decode_uint32(serialized + PREVIOUS_PID_OFFSET), page->previous);
    ASSERT_EQ(decode_uint32(serialized + NEXT_PID_OFFSET), page->next);
    ASSERT_EQ(decode_uint32(serialized + RIGHTMOST_PID_OFFSET), page->rightmost_ptr);
    ASSERT_EQ(decode_uint16(serialized + TREE_LEVEL_OFFSET), page->level);
    ASSERT_EQ(*(serialized + TREE_FLAGS_OFFSET), page->flags);

    // deserialize into BTreePage
    auto page_deserialized = new BTreePage(serialized);
    ASSERT_EQ(page_deserialized->rightmost_ptr, page->rightmost_ptr);
    ASSERT_EQ(page_deserialized->is_leaf, page->is_leaf);
    ASSERT_EQ(page_deserialized->previous, page->previous);
    ASSERT_EQ(page_deserialized->next, page->next);
    for (u8 i = 0; i < page_deserialized->keys.size(); i++) {
        // compare keys as they were added to keys read from end of page, same for values
        ASSERT_EQ(page_deserialized->keys.at(i).length, page->keys.at(i).length);
        ASSERT_EQ(*page_deserialized->keys.at(i).data, *page->keys.at(i).data);

        auto read_record = LEAF_RECORDS(page_deserialized->records).at(i);
        auto actual_record = LEAF_RECORDS(page->records).at(i);
        ASSERT_EQ(read_record.pid, actual_record.pid);
        ASSERT_EQ(read_record.slot_num, actual_record.slot_num);
    }
}

TEST_F(IndexTestFixture, InsertTest_RootWithEnoughSpace) {
    // check the tree metadata page (created with create_btree_index)
    u8 *metadata = read_page(BTREE_METADATA_PAGE_ID, bpm->disk_manager);
    ASSERT_EQ(decode_uint32(metadata + MAGIC_NUMBER_OFFSET), BTREE_INDEX);
    auto tree = new BTree(bpm);
    tree->deserialize();

    // insert kv pair
    std::string data1 = "1";
    BTreeKey key1 = {reinterpret_cast<u8 *>(data1.data()), static_cast<u8>(data1.length())};
    RID val1 = {.pid = 4, .slot_num = 5}; // doesn't need to be actual rid
    page_id_t leaf_pid = tree->insert(key1, val1);

    auto bpm_page = fetch_bpm_page(leaf_pid, tree->bpm);
    auto leaf = new BTreePage(bpm_page->data);

    ASSERT_EQ(leaf->is_leaf, true);
    ASSERT_EQ((char *)leaf->keys.at(0).data == data1, true);
    ASSERT_EQ(LEAF_RECORDS(leaf->records).at(0).pid, val1.pid);

    // insert another and check sorting
    std::string data2 = "0";
    BTreeKey key2 = {reinterpret_cast<u8 *>(data2.data()), static_cast<u8>(data2.length())};
    leaf_pid = tree->insert(key2, val1);
    bpm_page = fetch_bpm_page(leaf_pid, tree->bpm);
    leaf = new BTreePage(bpm_page->data);
    ASSERT_EQ((char *)leaf->keys.at(0).data == data2, true);
    ASSERT_EQ((char *)leaf->keys.at(1).data == data1, true);
    ASSERT_EQ(LEAF_RECORDS(leaf->records).at(0).pid, val1.pid);
    ASSERT_EQ(LEAF_RECORDS(leaf->records).at(1).pid, val1.pid);
}

//
// TEST_F(IndexTestFixture, InsertTest_FullRoot) {
//     auto tree = BTree(index_name, bpm, tree_max_size);
//     RID val1 = {.pid = 1, .slot_num = 2};
//     RID val2 = {.pid = 3, .slot_num = 4};
//     RID val3 = {.pid = 5, .slot_num = 6};
//     RID val4 = {.pid = 7, .slot_num = 8};
//     RID val5 = {.pid = 9, .slot_num = 10};
//     std::string data1 = "A";
//     std::string data2 = "B";
//     std::string data3 = "D";
//     std::string data4 = "E";
//     std::string data5 = "C";
//     BTreeKey key1 = {reinterpret_cast<u8 *>(data1.data()), static_cast<u16>(data1.length())};
//     BTreeKey key2 = {reinterpret_cast<u8 *>(data2.data()), static_cast<u16>(data2.length())};
//     BTreeKey key3 = {reinterpret_cast<u8 *>(data3.data()), static_cast<u16>(data3.length())};
//     BTreeKey key4 = {reinterpret_cast<u8 *>(data4.data()), static_cast<u16>(data4.length())};
//     BTreeKey key5 = {reinterpret_cast<u8 *>(data5.data()), static_cast<u16>(data5.length())};
//
//     tree.insert(key1, val1);
//     tree.insert(key2, val2);
//     tree.insert(key3, val3);
//     tree.insert(key4, val4);
//
//     // Test duplicate
//     auto pre_size = tree.root->keys.size();
//     bool ok = tree.insert(key1, val1);
//     ASSERT_EQ(ok, false);
//     ASSERT_EQ(tree.root->keys.size(), pre_size);
//
//     // Test node split
//     tree.insert(key5, val5);
//     ASSERT_EQ((char *)tree.root->keys.at(0).data == data5, true); // "C" in middle
//     ASSERT_EQ(tree.root->keys.size(), 1);
//     std::vector<BTreeKey> left_node_keys = {key1, key2};        // A B
//     std::vector<BTreeKey> right_node_keys = {key5, key3, key4}; // C D E
//     ASSERT_EQ(INTERNAL_CHILDREN(tree.root->children).at(0)->keys == left_node_keys, true);
//     ASSERT_EQ(INTERNAL_CHILDREN(tree.root->children).at(1)->keys == right_node_keys, true);
// }

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
} // namespace somedb
