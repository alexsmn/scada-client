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

std::shared_ptr<aui::TreeModel> MakeTreeModel() {
  auto root = std::make_unique<TestTreeNode>(u"Root");
  root->Add(0, std::make_unique<TestTreeNode>(u"Child"));
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
