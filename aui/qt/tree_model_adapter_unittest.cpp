#include "aui/qt/tree_model_adapter.h"

#include "aui/models/tree_model.h"
#include "aui/severity_colors.h"
#include "aui/test/app_environment.h"

#include <QFont>
#include <QVariant>
#include <gtest/gtest.h>

namespace scada::aui {
namespace {

// A root-only, two-column tree whose second column is a monospace value
// column (the Explorer's Name/Value shape).
class StubTreeModel : public TreeModel {
 public:
  virtual void* GetRoot() override { return &root_; }
  virtual int GetColumnCount() const override { return 2; }
  virtual void* GetParent(void* node) override {
    return node == &child_ ? &root_ : nullptr;
  }
  // One child, so a test can index a row that is not the root — the root is
  // deliberately never checkable.
  virtual int GetChildCount(void* parent) override {
    return parent == &root_ ? 1 : 0;
  }
  virtual void* GetChild(void* parent, int index) override { return &child_; }
  virtual bool HasChildren(void* parent) const override {
    return parent == const_cast<int*>(&root_);
  }
  virtual std::u16string GetText(void* node, int column_id) override {
    return u"1.5";
  }
  virtual bool IsMonospaceColumn(int column_id) const override {
    return column_id == 1;
  }

 private:
  int root_ = 0;
  int child_ = 1;
};

class TreeModelAdapterTest : public testing::Test {
 protected:
  void TearDown() override { SetSeverityTheme(SeverityTheme::kLegacy); }

  QVariant FontFor(int column) {
    return adapter_.data(adapter_.index(0, column), Qt::FontRole);
  }

  AppEnvironment app_env_;
  TreeModelAdapter adapter_{std::make_shared<StubTreeModel>()};
};

// Under a token theme the model-flagged value column renders in the
// fixed-pitch monospace value font; the name column keeps the default font.
TEST_F(TreeModelAdapterTest, MonospaceColumnRendersMonospaceUnderTokenTheme) {
  SetSeverityTheme(SeverityTheme::kDark);

  EXPECT_FALSE(FontFor(0).isValid());

  const QVariant value_font = FontFor(1);
  ASSERT_TRUE(value_font.isValid());
  EXPECT_TRUE(value_font.value<QFont>().fixedPitch());
}

// The legacy look is unchanged: no column supplies a custom font.
TEST_F(TreeModelAdapterTest, LegacyThemeKeepsTheDefaultFont) {
  EXPECT_FALSE(FontFor(0).isValid());
  EXPECT_FALSE(FontFor(1).isValid());
}

}  // namespace

// A checkable tree supplies Qt::CheckStateRole so the platform style draws an
// indicator on every row, checked or not — the shape
// docs/ui-mockups/screens/trend.html specifies, where an unchecked `.cb` is
// still a visible box. A tree that is not checkable supplies nothing, so no
// indicator column is reserved.
TEST_F(TreeModelAdapterTest, CheckStateIsSuppliedOnlyWhenCheckable) {
  // The root row never carries a box; use its child.
  const QModelIndex index = adapter_.index(0, 0, adapter_.index(0, 0));

  EXPECT_FALSE(adapter_.data(index, Qt::CheckStateRole).isValid());

  adapter_.SetCheckable(true);
  const QVariant checked = adapter_.data(index, Qt::CheckStateRole);
  ASSERT_TRUE(checked.isValid()) << "an unchecked row still needs its box";
  EXPECT_EQ(checked.toInt(), Qt::Unchecked);

  // And turning it back off withdraws it. Tree::SetShowChecks used to ignore
  // its argument and enable checks whatever it was passed.
  adapter_.SetCheckable(false);
  EXPECT_FALSE(adapter_.data(index, Qt::CheckStateRole).isValid());
}

}  // namespace scada::aui
