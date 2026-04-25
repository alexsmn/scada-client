#include "aui/qt/tree.h"

#include "aui/models/tree_node_model.h"
#include "aui/test/qt/app_environment.h"

#include <gtest/gtest.h>

namespace {

class TestTreeNode : public aui::TreeNode<TestTreeNode> {
 public:
  explicit TestTreeNode(std::u16string text) : text_{std::move(text)} {}

  std::u16string GetText(int /*column_id*/) const override { return text_; }

  bool HasChildren() const override { return GetChildCount() != 0; }

 private:
  std::u16string text_;
};

std::shared_ptr<aui::TreeNodeModel<TestTreeNode>> MakeTreeModel() {
  auto root = std::make_unique<TestTreeNode>(u"Root");
  auto child = std::make_unique<TestTreeNode>(u"Child");
  child->Add(0, std::make_unique<TestTreeNode>(u"Grandchild"));
  root->Add(0, std::make_unique<TestTreeNode>(u"First"));
  root->Add(1, std::move(child));
  root->Add(2, std::make_unique<TestTreeNode>(u"Last"));
  return std::make_shared<aui::TreeNodeModel<TestTreeNode>>(std::move(root));
}

}  // namespace

TEST(TreeTest, VisibleRootStaysDecoratedAndExpanded) {
  AppEnvironment app_env;

  aui::Tree tree{MakeTreeModel()};
  tree.SetRootVisible(true);

  const auto root_index = tree.model()->index(0, 0);
  ASSERT_TRUE(root_index.isValid());
  EXPECT_TRUE(tree.rootIsDecorated());
  EXPECT_TRUE(tree.isExpanded(root_index));
}

TEST(TreeTest, GetChildNodesReturnsModelNodesInViewOrder) {
  AppEnvironment app_env;

  auto model = MakeTreeModel();
  aui::Tree tree{model};
  auto* root = model->GetRoot();

  auto root_children = tree.GetChildNodes(root);

  ASSERT_EQ(3u, root_children.size());
  EXPECT_EQ(model->GetChild(root, 0), root_children[0]);
  EXPECT_EQ(model->GetChild(root, 1), root_children[1]);
  EXPECT_EQ(model->GetChild(root, 2), root_children[2]);

  auto child_children = tree.GetChildNodes(root_children[1]);

  ASSERT_EQ(1u, child_children.size());
  EXPECT_EQ(model->GetChild(root_children[1], 0), child_children[0]);
}

TEST(TreeTest, ExpandNodeExpandsMatchingTreeIndex) {
  AppEnvironment app_env;

  auto model = MakeTreeModel();
  aui::Tree tree{model};
  auto* child = model->GetChild(model->GetRoot(), 1);

  const auto child_index = tree.model()->index(1, 0, tree.rootIndex());
  ASSERT_TRUE(child_index.isValid());
  ASSERT_FALSE(tree.isExpanded(child_index));

  tree.ExpandNode(child);

  EXPECT_TRUE(tree.isExpanded(child_index));
}
