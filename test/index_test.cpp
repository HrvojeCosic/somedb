#include "../include/bpm.h"
#include "../include/index.hpp"
#include <gtest/gtest.h>

namespace somedb {

class IndexTestFixture : public testing::Test {
  protected:
    const char table_name[20] = "bplus_test_table";
    const char index_name[25] = "bplus_test_table_idx";
    BufferPoolManager *bpm;
    DiskManager *disk_mgr;

    const u16 tree_max_size = 5;

    void SetUp() override {
        char cname1[5] = "name";
        char cname2[5] = "age";
        Column cols[2] = {{.name_len = (u8)strlen(cname1), .name = cname1, .type = STRING},
                          {.name_len = (u8)strlen(cname2), .name = cname2, .type = UINT16}};
        disk_mgr = create_table(table_name, cols, (sizeof(cols) / sizeof(Column)));
        bpm = new_bpm(100, disk_mgr);
    }

    void TearDown() override { remove_table(table_name); }
};

TEST_F(IndexTestFixture, InsertTestRootWithEnoughSpace) {
    auto tree = BTree(index_name, bpm, tree_max_size);
    std::string data1 = "Perica";
    BTreeKey key1 = {reinterpret_cast<u8 *>(data1.data()), static_cast<u16>(data1.length())};
    RID val1 = {.pid = 4, .slot_num = 5}; // doesn't need to be actual rid
    tree.insert(key1, val1);

    ASSERT_EQ((char *)tree.root->keys.at(0).data == data1, true);
    ASSERT_EQ(LEAF_RECORDS(tree.root->records).at(0).pid, 4);

    std::string data2 = "Marica";
    BTreeKey key2 = {reinterpret_cast<u8 *>(data2.data()), static_cast<u16>(data2.length())};
    tree.insert(key2, val1);
    ASSERT_EQ((char *)tree.root->keys.at(0).data == data1, true);
    ASSERT_EQ((char *)tree.root->keys.at(1).data == data2, true);
    ASSERT_EQ(LEAF_RECORDS(tree.root->records).at(0).pid, val1.pid);
    ASSERT_EQ(LEAF_RECORDS(tree.root->records).at(1).pid, val1.pid);
}

TEST_F(IndexTestFixture, DISABLED_GetValue1) {}

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
} // namespace somedb
