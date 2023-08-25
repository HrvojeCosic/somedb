// https://refactoring.guru/design-patterns/strategy

#include "./btree_index.hpp"
#include "index_page.hpp"

namespace somedb {

struct MergeStrategy {
    virtual ~MergeStrategy() = default;
    virtual void merge(BTreePageLocalInfo &local, BTree &tree, BREADCRUMB_TYPE &parent_crumb, bool is_left_sib) = 0;
};

struct MergeContext {
  private:
    std::unique_ptr<MergeStrategy> strategy_;
    BTreePageLocalInfo &local;
    BTree &tree;
    BREADCRUMB_TYPE &parent_crumb;
    bool is_left_sib;

  public:
    explicit MergeContext(BTreePageLocalInfo &local, BTree &tree, BREADCRUMB_TYPE &crumb, bool is_left,
                          std::unique_ptr<MergeStrategy> strategy = {})
        : strategy_(std::move(strategy)), local(local), tree(tree), parent_crumb(crumb), is_left_sib(is_left) {}

    inline void setStrategy(std::unique_ptr<MergeStrategy> strategy) { strategy_ = std::move(strategy); }

    inline void merge() const { strategy_->merge(local, tree, parent_crumb, is_left_sib); }
};

struct MergeLeafNodeStrategy : MergeStrategy {
    // Merge leaf node with either sibling only if a node can hold up to N key-value pairs, and a combined number of
    // key-value pairs in two neighboring nodes is less than or equal to N."
    void merge(BTreePageLocalInfo &local, BTree &tree, BREADCRUMB_TYPE &parent_crumb, bool is_left_sib);
};

struct MergeNonLeafNodeStrategy : MergeStrategy {
    // Merge non-leaf node with either sibling only if a node can hold up to N + 1 pointers, and a combined number
    // of pointers in two neighboring nodes is less than or equal to N + 1.
    void merge(BTreePageLocalInfo &local, BTree &tree, BREADCRUMB_TYPE &parent_crumb, bool is_left_sib);
};

} // namespace somedb