#include "aui/qt/tree.h"

#include "aui/models/tree_node_model.h"
#include "aui/test/qt/app_environment.h"

#include <QApplication>
#include <QColor>
#include <QPalette>
#include <gtest/gtest.h>

namespace {

class TestTreeNode : public scada::aui::TreeNode<TestTreeNode> {
 public:
  explicit TestTreeNode(std::u16string text) : text_{std::move(text)} {}
  TestTreeNode(std::u16string text, scada::aui::Color text_color)
      : text_{std::move(text)}, text_color_{text_color} {}

  std::u16string GetText(int /*column_id*/) const override { return text_; }
  scada::aui::Color GetTextColor(int /*column_id*/) const override {
    return text_color_;
  }

  bool HasChildren() const override { return GetChildCount() != 0; }

 private:
  std::u16string text_;
  scada::aui::Color text_color_ = scada::aui::ColorCode::Transparent;
};

std::shared_ptr<scada::aui::TreeNodeModel<TestTreeNode>> MakeTreeModel() {
  auto root = std::make_unique<TestTreeNode>(u"Root");
  auto child = std::make_unique<TestTreeNode>(u"Child");
  child->Add(0, std::make_unique<TestTreeNode>(u"Grandchild"));
  root->Add(0, std::make_unique<TestTreeNode>(u"First"));
  root->Add(1, std::move(child));
  root->Add(2, std::make_unique<TestTreeNode>(u"Last"));
  return std::make_shared<scada::aui::TreeNodeModel<TestTreeNode>>(
      std::move(root));
}

std::shared_ptr<scada::aui::TreeNodeModel<TestTreeNode>>
MakeColoredTreeModel() {
  auto root = std::make_unique<TestTreeNode>(u"Root");
  root->Add(0, std::make_unique<TestTreeNode>(u"Default"));
  root->Add(1, std::make_unique<TestTreeNode>(u"Explicit",
                                              scada::aui::ColorCode::White));
  return std::make_shared<scada::aui::TreeNodeModel<TestTreeNode>>(
      std::move(root));
}

// A model that starts with nothing under its root, the way one backed by an
// async fetch does. Populate() adds a group carrying a child, with the
// surrounding notifications.
class LateFilledTreeModel : public scada::aui::TreeNodeModel<TestTreeNode> {
 public:
  LateFilledTreeModel()
      : scada::aui::TreeNodeModel<TestTreeNode>{
            std::make_unique<TestTreeNode>(u"Root")} {}

  void Populate() {
    auto group = std::make_unique<TestTreeNode>(u"Group");
    group->Add(0, std::make_unique<TestTreeNode>(u"Child"));
    Add(*root(), root()->GetChildCount(), std::move(group));
  }

  void Clear() {
    if (root()->GetChildCount() != 0)
      Remove(*root(), 0, root()->GetChildCount());
  }

  // Replaces the whole tree the way a model backed by one payload at a time
  // does (`modules/watch/frame_decode_tree_model.cpp` is the one in the
  // client): every index the view holds is invalidated.
  void Reset() {
    TreeModelResetting();
    set_root(std::make_unique<TestTreeNode>(u"Root"));
    TreeModelReset();
    Populate();
  }
};

}  // namespace

TEST(TreeTest, VisibleRootStaysDecoratedAndExpanded) {
  AppEnvironment app_env;

  scada::aui::Tree tree{MakeTreeModel()};
  tree.SetRootVisible(true);

  const auto root_index = tree.model()->index(0, 0);
  ASSERT_TRUE(root_index.isValid());
  EXPECT_TRUE(tree.rootIsDecorated());
  EXPECT_TRUE(tree.isExpanded(root_index));
}

// A hidden root is the view's root index, which a model reset invalidates. Left
// unrestored, the root row reappears above the tree — which is what every
// decode after the first drew in the protocol-trace pane before the pane
// re-hid it by hand.
TEST(TreeTest, HiddenRootStaysHiddenAcrossAModelReset) {
  AppEnvironment app_env;

  auto model = std::make_shared<LateFilledTreeModel>();
  model->Populate();
  scada::aui::Tree tree{model};
  ASSERT_TRUE(tree.rootIndex().isValid());
  // The root's children are the top level: one group, not one root row.
  ASSERT_EQ(tree.model()->rowCount(tree.rootIndex()), 1);

  model->Reset();

  EXPECT_TRUE(tree.rootIndex().isValid());
  EXPECT_EQ(tree.model()->rowCount(tree.rootIndex()), 1);
  EXPECT_EQ(tree.model()
                ->index(0, 0, tree.rootIndex())
                .data(Qt::DisplayRole)
                .toString(),
            QStringLiteral("Group"));
}

// The model's single top-level row is the tree's root and is never filtered
// away: it is the hidden root's persistent index, and losing it to a filter
// that matched nothing would bring the root row back once the filter cleared.
TEST(TreeTest, RootSurvivesAFilterThatMatchesNothing) {
  AppEnvironment app_env;

  scada::aui::Tree tree{MakeTreeModel()};
  const auto root_index = tree.rootIndex();
  ASSERT_TRUE(root_index.isValid());

  tree.SetFilterText(u"nothing matches this");
  EXPECT_TRUE(tree.rootIndex().isValid());
  EXPECT_EQ(tree.model()->rowCount(tree.rootIndex()), 0);

  tree.SetFilterText(u"");
  EXPECT_EQ(tree.rootIndex(), root_index);
  EXPECT_EQ(tree.model()->rowCount(tree.rootIndex()), 3);
}

TEST(TreeTest, GetChildNodesReturnsModelNodesInViewOrder) {
  AppEnvironment app_env;

  auto model = MakeTreeModel();
  scada::aui::Tree tree{model};
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
  scada::aui::Tree tree{model};
  auto* child = model->GetChild(model->GetRoot(), 1);

  const auto child_index = tree.model()->index(1, 0, tree.rootIndex());
  ASSERT_TRUE(child_index.isValid());
  ASSERT_FALSE(tree.isExpanded(child_index));

  tree.ExpandNode(child);

  EXPECT_TRUE(tree.isExpanded(child_index));
}

TEST(TreeTest, DefaultTextColorUsesPaletteForeground) {
  AppEnvironment app_env;

  scada::aui::Tree tree{MakeColoredTreeModel()};
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

  scada::aui::Tree tree{MakeColoredTreeModel()};

  EXPECT_EQ(tree.palette().color(QPalette::Base), QColor(32, 33, 36));
  EXPECT_EQ(tree.palette().color(QPalette::Text), QColor(232, 234, 237));
  EXPECT_EQ(tree.palette().color(QPalette::AlternateBase), QColor(32, 33, 36));
}

TEST(TreeTest, ExplicitTextColorOverridesPaletteForeground) {
  AppEnvironment app_env;

  scada::aui::Tree tree{MakeColoredTreeModel()};
  const auto explicit_index = tree.model()->index(1, 0, tree.rootIndex());

  ASSERT_TRUE(tree.model()->data(explicit_index, Qt::ForegroundRole).isValid());
  EXPECT_EQ(
      tree.model()->data(explicit_index, Qt::ForegroundRole).value<QColor>(),
      QColor(Qt::white));
}

TEST(TreeTest, SetFilterTextHidesNonMatchingRowsAndClears) {
  AppEnvironment app_env;

  scada::aui::Tree tree{MakeTreeModel()};
  // Top-level rows: First, Child, Last.
  EXPECT_EQ(tree.model()->rowCount(tree.rootIndex()), 3);

  tree.SetFilterText(u"first");
  EXPECT_EQ(tree.model()->rowCount(tree.rootIndex()), 1);

  tree.SetFilterText(u"");  // Empty filter restores every row.
  EXPECT_EQ(tree.model()->rowCount(tree.rootIndex()), 3);
}

TEST(TreeTest, SetFilterTextIsCaseInsensitive) {
  AppEnvironment app_env;

  scada::aui::Tree tree{MakeTreeModel()};
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

  scada::aui::Tree tree{MakeTreeModel()};
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

// The regression behind task 126: the node-properties tree called expandAll()
// while its model was still empty — NodePropertyModel fetches the node and its
// type chain before it has any properties — so every group that arrived a
// moment later came up collapsed and stayed that way.
TEST(TreeTest, ExpandAllWhenPopulatedExpandsRowsThatArriveLater) {
  AppEnvironment app_env;

  auto model = std::make_shared<LateFilledTreeModel>();
  scada::aui::Tree tree{model};

  tree.ExpandAllWhenPopulated();
  ASSERT_EQ(tree.model()->rowCount(tree.rootIndex()), 0);

  model->Populate();

  const auto group_index = tree.model()->index(0, 0, tree.rootIndex());
  ASSERT_TRUE(group_index.isValid());
  EXPECT_TRUE(tree.isExpanded(group_index));
}

TEST(TreeTest, ExpandAllWhenPopulatedExpandsRowsThatArePresentAlready) {
  AppEnvironment app_env;

  auto model = std::make_shared<LateFilledTreeModel>();
  model->Populate();
  scada::aui::Tree tree{model};

  tree.ExpandAllWhenPopulated();

  EXPECT_TRUE(tree.isExpanded(tree.model()->index(0, 0, tree.rootIndex())));
}

// One-shot: it is "expand on first open", not "keep re-expanding". A later
// repopulation must not overrule a group the operator has since collapsed.
TEST(TreeTest, ExpandAllWhenPopulatedDoesNotReExpandOnALaterRepopulation) {
  AppEnvironment app_env;

  auto model = std::make_shared<LateFilledTreeModel>();
  scada::aui::Tree tree{model};

  tree.ExpandAllWhenPopulated();
  model->Populate();
  ASSERT_TRUE(tree.isExpanded(tree.model()->index(0, 0, tree.rootIndex())));

  tree.collapse(tree.model()->index(0, 0, tree.rootIndex()));
  model->Clear();
  model->Populate();

  EXPECT_FALSE(tree.isExpanded(tree.model()->index(0, 0, tree.rootIndex())));
}
