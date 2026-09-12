#include "aui/qt/tree.h"

#include "aui/models/tree_model.h"
#include "aui/test/app_environment.h"

#include <QApplication>

#include <gtest/gtest.h>

#include <algorithm>
#include <memory>
#include <string>
#include <vector>

namespace scada::aui {
namespace {

// A flat tree under one root whose row texts a test can change afterwards,
// which is how the Explorer's rows really behave: a row is materialized as
// soon as the browse names it, and its display name lands later when the
// attribute fetch completes.
class MutableTreeModel : public TreeModel {
 public:
  struct Row {
    std::u16string text;
    // Stands in for the node attributes the Explorer sorts on ahead of the
    // name: NodeClass (folders before variables) and TypeDefinition. Both are
    // unknown until the node's own fetch lands, so both start false.
    bool is_leaf = false;
  };

  // Appends a row and announces it the way the configuration tree does.
  Row* AddRow(std::u16string text) {
    const int index = static_cast<int>(rows_.size());
    TreeNodesAdding(&root_, index, 1);
    rows_.push_back(std::make_unique<Row>(Row{std::move(text)}));
    TreeNodesAdded(&root_, index, 1);
    return rows_.back().get();
  }

  // Renames a row and notifies, standing in for a completed fetch.
  void Rename(Row* row, std::u16string text) {
    row->text = std::move(text);
    TreeNodeChanged(row);
  }

  // A completed fetch that answers only the grouping attributes: the name was
  // already known from the browse, so the text does not change. This is the
  // Explorer's ordinary case and it is what moves a row between the folder and
  // the leaf group.
  void Classify(Row* row, bool is_leaf) {
    row->is_leaf = is_leaf;
    TreeNodeChanged(row);
  }

  // TreeModel
  virtual void* GetRoot() override { return &root_; }
  virtual void* GetParent(void* node) override {
    return node == &root_ ? nullptr : &root_;
  }
  virtual int GetChildCount(void* parent) override {
    return parent == &root_ ? static_cast<int>(rows_.size()) : 0;
  }
  virtual void* GetChild(void* parent, int index) override {
    return parent == &root_ ? rows_[index].get() : nullptr;
  }
  virtual bool HasChildren(void* parent) const override {
    return parent == const_cast<int*>(&root_);
  }
  virtual std::u16string GetText(void* node, int column_id) override {
    return node == &root_ ? u"root" : static_cast<Row*>(node)->text;
  }

 private:
  int root_ = 0;
  std::vector<std::unique_ptr<Row>> rows_;
};

// Orders rows by text, the shape every caller of SetCompareHandler uses.
int CompareByText(void* left, void* right) {
  const auto& a = static_cast<MutableTreeModel::Row*>(left)->text;
  const auto& b = static_cast<MutableTreeModel::Row*>(right)->text;
  return a == b ? 0 : (a < b ? -1 : 1);
}

// Groups before names, the shape CompareNodes uses: folders first, then by
// name. Both terms read state that arrives with the node's fetch.
int CompareByGroupThenText(void* left, void* right) {
  const auto* a = static_cast<MutableTreeModel::Row*>(left);
  const auto* b = static_cast<MutableTreeModel::Row*>(right);
  if (a->is_leaf != b->is_leaf)
    return a->is_leaf ? 1 : -1;
  return a->text == b->text ? 0 : (a->text < b->text ? -1 : 1);
}

// The root's child texts in the order the view shows them.
std::vector<std::u16string> VisibleTexts(const Tree& tree,
                                         MutableTreeModel& model) {
  std::vector<std::u16string> texts;
  for (void* node : tree.GetChildNodes(model.GetRoot()))
    texts.push_back(static_cast<MutableTreeModel::Row*>(node)->text);
  return texts;
}

class TreeTest : public testing::Test {
 protected:
  AppEnvironment app_environment_;
};

// Rows already in the model when the comparator is installed must be reordered
// by it. ConfigurationTreeView calls SetSorted(true) and SetCompareHandler in
// that order, and a tree restored from a saved profile can already hold rows by
// then -- so a comparator that only governs later inserts leaves the rows an
// operator actually sees in browse order.
TEST_F(TreeTest, InstallingTheComparatorReordersRowsAlreadyPresent) {
  auto model = std::make_shared<MutableTreeModel>();
  model->AddRow(u"Charlie");
  model->AddRow(u"Alpha");
  model->AddRow(u"Bravo");

  Tree tree{model};
  tree.SetSorted(true);
  tree.SetCompareHandler(&CompareByText);

  EXPECT_EQ(VisibleTexts(tree, *model),
            (std::vector<std::u16string>{u"Alpha", u"Bravo", u"Charlie"}));
}

// The Explorer's rows are inserted before their display names arrive, so every
// row is first compared under a placeholder and then renamed. The order the
// operator ends up with must be the order of the final names and must not
// depend on which fetch answered first -- the defect behind visual_review's
// "devices.png row order changes between renders".
TEST_F(TreeTest, RowOrderFollowsFinalNamesNotArrivalOrder) {
  auto model = std::make_shared<MutableTreeModel>();
  Tree tree{model};
  tree.SetSorted(true);
  tree.SetCompareHandler(&CompareByText);

  // Every row materializes carrying the same placeholder, which is what makes
  // the initial comparison say nothing: the rows land in browse order.
  std::vector<MutableTreeModel::Row*> rows;
  for (int i = 0; i < 5; ++i)
    rows.push_back(model->AddRow(u"[Loading]"));

  // The fetches answer out of order, as they do over a real session.
  model->Rename(rows[2], u"Echo");
  model->Rename(rows[0], u"Bravo");
  model->Rename(rows[4], u"Alpha");
  model->Rename(rows[1], u"Delta");
  model->Rename(rows[3], u"Charlie");

  EXPECT_EQ(VisibleTexts(tree, *model),
            (std::vector<std::u16string>{u"Alpha", u"Bravo", u"Charlie",
                                         u"Delta", u"Echo"}));
}

// The Explorer sorts on the node's NodeClass and TypeDefinition before its
// name, and all three arrive with the node's fetch -- so every row is first
// placed under attributes it does not have yet, and is re-placed once they
// land. The rows must end up grouped and named in order however those fetches
// interleave. This is the shape behind devices.png's row order changing
// between renders.
TEST_F(TreeTest, RowsGroupByAttributesThatArriveAfterTheRowDoes) {
  auto model = std::make_shared<MutableTreeModel>();
  Tree tree{model};
  tree.SetSorted(true);
  tree.SetCompareHandler(&CompareByGroupThenText);

  // The browse names every child, so the rows carry their final text from the
  // start; what they lack is the class that decides which group they sit in.
  struct Seed {
    std::u16string text;
    bool is_leaf;
  };
  const std::vector<Seed> seeds = {
      {u"Delta", false}, {u"Ua", true},      {u"Alpha", false},
      {u"Total", true},  {u"Charlie", false}};

  std::vector<MutableTreeModel::Row*> rows;
  for (const Seed& seed : seeds)
    rows.push_back(model->AddRow(seed.text));

  // The fetches answer in an order of their own, which is the part no session
  // controls.
  for (int i : {3, 0, 4, 1, 2})
    model->Classify(rows[i], seeds[i].is_leaf);

  EXPECT_EQ(VisibleTexts(tree, *model),
            (std::vector<std::u16string>{u"Alpha", u"Charlie", u"Delta",
                                         u"Total", u"Ua"}));
}

}  // namespace
}  // namespace scada::aui
