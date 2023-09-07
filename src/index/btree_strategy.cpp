#include "../../include/index/btree_strategy.hpp"
#include "../../include/index/btree_index.hpp"
#include "../../include/index/index_page.hpp"

namespace somedb {
void MergeLeafNodeStrategy::merge(BTreePageLocalInfo &local, BTree &tree, std::pair<page_id_t, int> &parent_crumb,
                                  bool is_left_sib) {
    auto &sibling_vals = LEAF_RECORDS(local.sibling->values);
    auto &node_vals = LEAF_RECORDS(local.node->values);
    auto &parent_vals = INTERNAL_CHILDREN(local.parent->values);

    // Move all elements to node that is more left of the two,
    // and remove the child pointer of the leaf node that lost its elements from the parent node.
    // This is done differently depending on if the sibling is left or right
    if (!local.sibling->keys.size() || local.node->keys.size() + local.sibling->keys.size() > tree.max_size)
        return;

    if (is_left_sib) {
        local.sibling->keys.insert(local.sibling->keys.end(), local.node->keys.begin(), local.node->keys.end());
        sibling_vals.insert(sibling_vals.end(), node_vals.begin(), node_vals.end());
        BTree::flush_node(local.sibling_pid, local.sibling->serialize(), tree.bpm);

        if (local.parent->keys.size() == 1) {
            INTERNAL_CHILDREN(local.parent->values).at(0) = local.sibling_pid;
        } else {
            local.parent->rightmost_ptr = local.sibling_pid;
        }
        INTERNAL_CHILDREN(local.parent->values).pop_back();
        local.parent->keys.pop_back();
    } else {
        local.node->keys.insert(local.node->keys.end(), local.sibling->keys.begin(), local.sibling->keys.end());
        node_vals.insert(node_vals.end(), sibling_vals.begin(), sibling_vals.end());

        if (local.sibling_pid == local.parent->rightmost_ptr) {
            local.parent->rightmost_ptr = 0;
        } else {
            local.node->next = local.sibling->next;
            parent_vals.erase(parent_vals.begin() + parent_crumb.second);
            parent_vals.at(parent_crumb.second) = local.node_pid;
            local.parent->keys.erase(local.parent->keys.begin() + parent_crumb.second);
        }
        BTree::flush_node(local.node_pid, local.node->serialize(), tree.bpm);
    }
};

void MergeNonLeafNodeStrategy::merge(BTreePageLocalInfo &local, BTree &tree, BREADCRUMB_TYPE &parent_crumb,
                                     bool is_left_sib) {
    u16 total_ptr_num = local.node->keys.size() + 1 + local.sibling->keys.size() + 1; // +1 for rightmost pointer
    if (total_ptr_num > tree.max_size + 1)
        return;

    auto &sibling_vals = INTERNAL_CHILDREN(local.sibling->values);
    auto &node_vals = INTERNAL_CHILDREN(local.node->values);

    if (is_left_sib) {
        // The separator key is the last key because we came to this level from the rightmsot pointer
        auto demoted_key = local.parent->keys.at(local.parent->keys.size() - 1);
        local.parent->keys.erase(local.parent->keys.begin() + local.parent->keys.size() - 1);
        local.parent->rightmost_ptr = 0;

        local.sibling->keys.insert(local.sibling->keys.end(), demoted_key);
        local.sibling->keys.insert(local.sibling->keys.end(), local.node->keys.begin(), local.node->keys.end());

        sibling_vals.emplace_back(local.sibling->rightmost_ptr);
        sibling_vals.insert(sibling_vals.end(), node_vals.begin(), node_vals.end());
        local.sibling->rightmost_ptr = local.node->rightmost_ptr;
        BTree::flush_node(local.sibling_pid, local.sibling->serialize(), tree.bpm);
    } else {
        auto demoted_key = local.parent->keys.at(parent_crumb.second);
        local.parent->keys.erase(local.parent->keys.begin() + parent_crumb.second);

        local.node->keys.insert(local.node->keys.end(), demoted_key);
        local.node->keys.insert(local.node->keys.end(), local.sibling->keys.begin(), local.sibling->keys.end());

        node_vals.insert(node_vals.end(), sibling_vals.begin(), sibling_vals.end());
        local.node->rightmost_ptr = local.sibling->rightmost_ptr;
        BTree::flush_node(local.node_pid, local.node->serialize(), tree.bpm);
    }

    BTree::flush_node(parent_crumb.first, local.parent->serialize(), tree.bpm);
}
} // namespace somedb