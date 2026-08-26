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

}  // namespace scada::aui
