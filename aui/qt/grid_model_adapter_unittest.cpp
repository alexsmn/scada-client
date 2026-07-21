#include "aui/qt/grid_model_adapter.h"

#include "aui/models/grid_model.h"
#include "aui/models/header_model.h"
#include "aui/severity_colors.h"
#include "aui/test/app_environment.h"

#include <gtest/gtest.h>

#include <QColor>
#include <QVariant>

#include <memory>

namespace scada::aui {
namespace {

// A one-cell model whose colours the test controls (the row count comes from
// the row header model).
class StubGridModel : public GridModel {
 public:
  virtual void GetCell(GridCell& cell) override {
    cell.text = u"42";
    cell.text_color = text_color;
    cell.cell_color = cell_color;
  }

  Color text_color = ColorCode::Transparent;
  Color cell_color = ColorCode::Transparent;
};

class GridModelAdapterTest : public testing::Test {
 protected:
  GridModelAdapterTest() {
    TableColumn column{0, u"C", 100};
    columns_->SetColumns(1, &column);
    rows_->SetColumnCount(1, 100);
  }

  void TearDown() override { SetSeverityTheme(SeverityTheme::kLegacy); }

  QVariant Data(int role) { return adapter_.data(adapter_.index(0, 0), role); }

  AppEnvironment app_env_;
  std::shared_ptr<StubGridModel> model_ = std::make_shared<StubGridModel>();
  std::shared_ptr<ColumnHeaderModel> rows_ =
      std::make_shared<ColumnHeaderModel>();
  std::shared_ptr<ColumnHeaderModel> columns_ =
      std::make_shared<ColumnHeaderModel>();
  GridModelAdapter adapter_{model_, rows_, columns_};
};

// The legacy look stays pixel-identical: unstyled cells render the historical
// black-on-white defaults.
TEST_F(GridModelAdapterTest, LegacyUnstyledCellsRenderBlackOnWhite) {
  EXPECT_EQ(Data(Qt::ForegroundRole).value<QColor>(), QColor{Qt::black});
  EXPECT_EQ(Data(Qt::BackgroundRole).value<QColor>(), QColor{Qt::white});
}

// Under the reshell theme, unstyled cells fall through to the theme palette
// (regression: the hardcoded white/black defaults left every grid surface -
// transmission rules, node tables - white under the dark theme).
TEST_F(GridModelAdapterTest, ThemedUnstyledCellsFallThroughToThePalette) {
  SetSeverityTheme(SeverityTheme::kDark);
  EXPECT_FALSE(Data(Qt::ForegroundRole).isValid());
  EXPECT_FALSE(Data(Qt::BackgroundRole).isValid());
}

// Explicit colours always pass through.
TEST_F(GridModelAdapterTest, ExplicitColoursPassThrough) {
  model_->text_color = ColorCode::Red;
  model_->cell_color = ColorCode::Yellow;
  EXPECT_EQ(Data(Qt::ForegroundRole).value<QColor>(), QColor{Qt::red});
  EXPECT_EQ(Data(Qt::BackgroundRole).value<QColor>(), QColor{Qt::yellow});

  SetSeverityTheme(SeverityTheme::kDark);
  EXPECT_EQ(Data(Qt::ForegroundRole).value<QColor>(), QColor{Qt::red});
  EXPECT_EQ(Data(Qt::BackgroundRole).value<QColor>(), QColor{Qt::yellow});
}

// A themed cell with an explicit background but default text derives a
// contrasting text colour, so a semantically light cell (read-only grey,
// blink yellow) stays readable on the dark theme.
TEST_F(GridModelAdapterTest, ThemedExplicitBackgroundDerivesContrastingText) {
  SetSeverityTheme(SeverityTheme::kDark);

  model_->cell_color = ColorCode::Yellow;  // light
  EXPECT_EQ(Data(Qt::ForegroundRole).value<QColor>(), QColor{Qt::black});

  model_->cell_color = Rgba{0x20, 0x20, 0x20};  // dark
  EXPECT_EQ(Data(Qt::ForegroundRole).value<QColor>(), QColor{Qt::white});
}

}  // namespace
}  // namespace scada::aui
