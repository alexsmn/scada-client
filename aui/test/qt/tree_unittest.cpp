#include "aui/qt/tree.h"

#include "aui/models/tree_node_model.h"
#include "aui/test/qt/app_environment.h"

#include <QApplication>
#include <QColor>
#include <QPalette>
#include <gtest/gtest.h>

namespace {

class TestTreeNode : public aui::TreeNode<TestTreeNode> {
 public:
  explicit TestTreeNode(std::u16string text) : text_{std::move(text)} {}
  TestTreeNode(std::u16string text, aui::Color text_color)
      : text_{std::move(text)}, text_color_{text_color} {}

  std::u16string GetText(int /*column_id*/) const override { return text_; }
  aui::Color GetTextColor(int /*column_id*/) const override {
    return text_color_;
  }

  bool HasChildren() const override { return GetChildCount() != 0; }

 private:
  std::u16string text_;
  aui::Color text_color_ = aui::ColorCode::Transparent;
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

std::shared_ptr<aui::TreeNodeModel<TestTreeNode>> MakeColoredTreeModel() {
  auto root = std::make_unique<TestTreeNode>(u"Root");
  root->Add(0, std::make_unique<TestTreeNode>(u"Default"));
  root->Add(1,
            std::make_unique<TestTreeNode>(u"Explicit", aui::ColorCode::White));
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

TEST(TreeTest, DefaultTextColorUsesPaletteForeground) {
  AppEnvironment app_env;

  aui::Tree tree{MakeColoredTreeModel()};
  const auto default_index = tree.model()->index(0, 0, tree.rootIndex());

  EXPECT_FALSE(tree.model()->data(default_index, Qt::ForegroundRole).isValid());
}

TEST(TreeTest, DefaultItemPaletteUsesWindowThemeColors) {
  AppEnvironment app_env;

  QPalette palette = QApplication::palette();
  for (auto group :
       {QPalette::Active, QPalette::Inactive, QPalette::Disabled}) {
    palette.setColor(group, QPalette::Window, QColor{32, 33, 36});
    palette.setColor(group, QPalette::WindowText, QColor{232, 234, 237});
    palette.setColor(group, QPalette::Base, Qt::white);
    palette.setColor(group, QPalette::Text, Qt::black);
  }
  QApplication::setPalette(palette);

  aui::Tree tree{MakeColoredTreeModel()};

  EXPECT_EQ(tree.palette().color(QPalette::Base), QColor(32, 33, 36));
  EXPECT_EQ(tree.palette().color(QPalette::Text), QColor(232, 234, 237));
  EXPECT_EQ(tree.palette().color(QPalette::AlternateBase), QColor(32, 33, 36));
}

TEST(TreeTest, ExplicitTextColorOverridesPaletteForeground) {
  AppEnvironment app_env;

  aui::Tree tree{MakeColoredTreeModel()};
  const auto explicit_index = tree.model()->index(1, 0, tree.rootIndex());

  ASSERT_TRUE(tree.model()->data(explicit_index, Qt::ForegroundRole).isValid());
  EXPECT_EQ(
      tree.model()->data(explicit_index, Qt::ForegroundRole).value<QColor>(),
      QColor(Qt::white));
}

TEST(TreeTest, SetFilterTextHidesNonMatchingRowsAndClears) {
  AppEnvironment app_env;

  aui::Tree tree{MakeTreeModel()};
  // Top-level rows: First, Child, Last.
  EXPECT_EQ(tree.model()->rowCount(tree.rootIndex()), 3);

  tree.SetFilterText(u"first");
  EXPECT_EQ(tree.model()->rowCount(tree.rootIndex()), 1);

  tree.SetFilterText(u"");  // Empty filter restores every row.
  EXPECT_EQ(tree.model()->rowCount(tree.rootIndex()), 3);
}

TEST(TreeTest, SetFilterTextIsCaseInsensitive) {
  AppEnvironment app_env;

  aui::Tree tree{MakeTreeModel()};
  tree.SetFilterText(u"LAST");

  ASSERT_EQ(tree.model()->rowCount(tree.rootIndex()), 1);
  EXPECT_EQ(tree.model()
                ->index(0, 0, tree.rootIndex())
                .data(Qt::DisplayRole)
                .toString(),
            QStringLiteral("Last"));
}

TEST(TreeTest, SetFilterTextKeepsAncestorsOfDeeperMatches) {
  AppEnvironment app_env;

  aui::Tree tree{MakeTreeModel()};
  // "Grandchild" lives under "Child"; filtering for it keeps "Child" visible as
  // the ancestor of the match, even though "Child" itself does not match.
  tree.SetFilterText(u"grandchild");

  ASSERT_EQ(tree.model()->rowCount(tree.rootIndex()), 1);
  EXPECT_EQ(tree.model()
                ->index(0, 0, tree.rootIndex())
                .data(Qt::DisplayRole)
                .toString(),
            QStringLiteral("Child"));
}
