#include "aui/qt/tree_model_adapter.h"

#include "aui/models/tree_model.h"
#include "aui/severity_colors.h"
#include "aui/test/app_environment.h"

#include <QApplication>
#include <QFont>
#include <QPalette>
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

// A root-only tree whose colour answer is whatever a test sets: either a
// `ColorRole` (resolved against the palette) or a literal `Color` (a process
// semantic that must survive untouched).
class ColorStubTreeModel : public TreeModel {
 public:
  ColorRole role = ColorRole::Default;
  Color text_color = ColorCode::Transparent;
  Color background_color = ColorCode::Transparent;

  virtual void* GetRoot() override { return &root_; }
  virtual void* GetParent(void* node) override { return nullptr; }
  virtual int GetChildCount(void* parent) override { return 0; }
  virtual bool HasChildren(void* parent) const override { return false; }
  virtual std::u16string GetText(void* node, int column_id) override {
    return u"value";
  }
  virtual ColorRole GetColorRole(void* node, int column_id) override {
    return role;
  }
  virtual Color GetTextColor(void* node, int column_id) override {
    return text_color;
  }
  virtual Color GetBackgroundColor(void* node, int column_id) override {
    return background_color;
  }

 private:
  int root_ = 0;
};

class TreeModelAdapterColorTest : public testing::Test {
 protected:
  QVariant Foreground() {
    return adapter_.data(adapter_.index(0, 0), Qt::ForegroundRole);
  }
  QVariant Background() {
    return adapter_.data(adapter_.index(0, 0), Qt::BackgroundRole);
  }

  AppEnvironment app_env_;
  std::shared_ptr<ColorStubTreeModel> model_ =
      std::make_shared<ColorStubTreeModel>();
  TreeModelAdapter adapter_{model_};
};

// The point of the role: a disabled cell takes the platform's disabled text
// colour, which follows the OS light/dark theme, instead of the fixed
// `ColorCode::Gray` (`{136, 136, 126}`) the property tree used to name.
TEST_F(TreeModelAdapterColorTest,
       DisabledRoleResolvesToThePaletteNotAFixedGrey) {
  model_->role = ColorRole::Disabled;

  const QColor expected =
      QApplication::palette().color(QPalette::Disabled, QPalette::Text);
  EXPECT_EQ(Foreground().value<QColor>(), expected);
  EXPECT_NE(Foreground().value<QColor>(), QColor(136, 136, 126));
}

// A heading takes both halves from the palette, where the old code named
// white-on-grey outright.
TEST_F(TreeModelAdapterColorTest, HeaderRoleTakesBothHalvesFromThePalette) {
  model_->role = ColorRole::Header;

  const QPalette& palette = QApplication::palette();
  EXPECT_EQ(Foreground().value<QColor>(),
            palette.color(QPalette::Normal, QPalette::ButtonText));
  EXPECT_EQ(Background().value<QColor>(),
            palette.color(QPalette::Normal, QPalette::Button));
}

// `Default` means "the model is not asking for anything", so the view keeps
// its own colours and the adapter supplies no override.
TEST_F(TreeModelAdapterColorTest, DefaultRoleSuppliesNoColor) {
  EXPECT_FALSE(Foreground().isValid());
  EXPECT_FALSE(Background().isValid());
}

// Alarm state and data quality are ISA-101/ISA-18.2 signals with fixed values
// that must not follow the platform theme. Those models leave the role at
// `Default` and name the colour outright, and the adapter must pass it
// through untouched rather than substituting a palette colour.
TEST_F(TreeModelAdapterColorTest, LiteralProcessColorSurvivesTheRoleLookup) {
  model_->text_color = ColorCode::Red;
  model_->background_color = ColorCode::Crimson;

  EXPECT_EQ(Foreground().value<QColor>(), QColor(255, 0, 0));
  EXPECT_EQ(Background().value<QColor>(), QColor(220, 20, 60));
}

class TreeModelAdapterTest : public testing::Test {
 protected:
  void TearDown() override { SetSeverityTheme(SeverityTheme::kDark); }

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

// A leaf must say so at the item, not only through hasChildren(). QTreeView
// caches hasChildren per laid-out row and refreshes it from dataChanged only
// for column 0, whereas it reads Qt::ItemNeverHasChildren live — in
// hasVisibleChildren, in layout() and in expand(). Without the flag a data-item
// row could keep an expander the model no longer claims.
TEST_F(TreeModelAdapterTest, LeafRowsCarryItemNeverHasChildren) {
  const QModelIndex root = adapter_.index(0, 0);
  const QModelIndex leaf = adapter_.index(0, 0, root);

  EXPECT_FALSE(adapter_.flags(root).testFlag(Qt::ItemNeverHasChildren))
      << "a row the model says has children must stay expandable";
  EXPECT_TRUE(adapter_.flags(leaf).testFlag(Qt::ItemNeverHasChildren));
}

// The flag is per row, not per column: it is derived from the node, and Qt only
// consults column 0, so the two must not disagree about the same node.
TEST_F(TreeModelAdapterTest, ItemNeverHasChildrenIsTheSameInEveryColumn) {
  const QModelIndex root = adapter_.index(0, 0);

  EXPECT_EQ(adapter_.flags(adapter_.index(0, 0, root))
                .testFlag(Qt::ItemNeverHasChildren),
            adapter_.flags(adapter_.index(0, 1, root))
                .testFlag(Qt::ItemNeverHasChildren));
}

}  // namespace

// A checkable tree supplies Qt::CheckStateRole so the platform style draws an
// indicator on every row, checked or not — the shape
// docs/product/ui-mockups/screens/trend.html specifies, where an unchecked
// `.cb` is still a visible box. A tree that is not checkable supplies nothing,
// so no indicator column is reserved.
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

// Qt's model contract allows an invalid index in data() and flags() — a
// QAbstractProxyModel forwards one unchanged for the viewport outside any row,
// e.g. a drag over the empty area — and both used to fail-stop on it. Neither
// is a caller bug: there is no data, and the base flags are the answer.
TEST_F(TreeModelAdapterTest, InvalidIndexHasNoDataAndBaseFlags) {
  EXPECT_FALSE(adapter_.data(QModelIndex{}, Qt::DisplayRole).isValid());
  EXPECT_EQ(adapter_.flags(QModelIndex{}),
            adapter_.QAbstractItemModel::flags(QModelIndex{}));
}

}  // namespace scada::aui
