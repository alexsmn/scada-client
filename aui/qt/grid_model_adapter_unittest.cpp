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
    ++get_cell_calls;
    cell.text = u"42";
    cell.text_color = text_color;
    cell.cell_color = cell_color;
    cell.alignment = alignment;
  }

  Color text_color = ColorCode::Transparent;
  Color cell_color = ColorCode::Transparent;
  std::optional<TableColumn::Alignment> alignment;
  int get_cell_calls = 0;
};

// A column header whose alignment the test controls.
class StubColumnModel : public ColumnHeaderModel {
 public:
  virtual TableColumn::Alignment GetAlignment(int index) const override {
    return alignment;
  }

  TableColumn::Alignment alignment = TableColumn::LEFT;
};

class GridModelAdapterTest : public testing::Test {
 protected:
  GridModelAdapterTest() {
    TableColumn column{0, u"C", 100};
    columns_->SetColumns(1, &column);
    rows_->SetColumnCount(1, 100);
  }

  void TearDown() override { SetSeverityTheme(SeverityTheme::kDark); }

  QVariant Data(int role) { return adapter_.data(adapter_.index(0, 0), role); }

  AppEnvironment app_env_;
  std::shared_ptr<StubGridModel> model_ = std::make_shared<StubGridModel>();
  std::shared_ptr<ColumnHeaderModel> rows_ =
      std::make_shared<ColumnHeaderModel>();
  std::shared_ptr<StubColumnModel> columns_ =
      std::make_shared<StubColumnModel>();
  GridModelAdapter adapter_{model_, rows_, columns_};
};

// Unstyled cells fall through to the theme palette. Regression: the adapter
// used to answer hardcoded white/black defaults, which left every grid
// surface — transmission rules, node tables — white under a dark theme.
TEST_F(GridModelAdapterTest, UnstyledCellsFallThroughToThePalette) {
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

// A column's alignment reaches Qt as Qt's own flags. Regression: the adapter
// returned the aui enum raw, and Qt read it as a flag mask — LEFT(0) became
// "no alignment" and RIGHT(1)/CENTER(2) became AlignLeft/AlignRight, so every
// grid in the client rendered left-aligned regardless of what its columns
// asked for.
TEST_F(GridModelAdapterTest, ColumnAlignmentReachesQtAsQtFlags) {
  columns_->alignment = TableColumn::RIGHT;
  EXPECT_EQ(Data(Qt::TextAlignmentRole).toInt(),
            static_cast<int>(Qt::AlignRight | Qt::AlignVCenter));

  columns_->alignment = TableColumn::CENTER;
  EXPECT_EQ(Data(Qt::TextAlignmentRole).toInt(),
            static_cast<int>(Qt::AlignHCenter | Qt::AlignVCenter));

  columns_->alignment = TableColumn::LEFT;
  EXPECT_EQ(Data(Qt::TextAlignmentRole).toInt(),
            static_cast<int>(Qt::AlignLeft | Qt::AlignVCenter));
}

// A cell may override its column — how the spreadsheet's per-cell alignment
// reaches the screen. Regression: GridCell carried no alignment, so the
// sheet's stored formats were silently dropped at render time.
TEST_F(GridModelAdapterTest, CellAlignmentOverridesItsColumn) {
  columns_->alignment = TableColumn::LEFT;
  model_->alignment = TableColumn::RIGHT;
  EXPECT_EQ(Data(Qt::TextAlignmentRole).toInt(),
            static_cast<int>(Qt::AlignRight | Qt::AlignVCenter));
}

// A delegate asks for seven roles per paint and per sizeHint; the model's
// GetCell formats the value each time. Only the roles that read the cell may
// pay for it — the adapter used to fetch first and switch on the role after.
TEST_F(GridModelAdapterTest, RolesThatDoNotReadTheCellDoNotFetchIt) {
  Data(Qt::SizeHintRole);
  Data(Qt::CheckStateRole);
  Data(Qt::FontRole);
  Data(Qt::DecorationRole);
  EXPECT_EQ(model_->get_cell_calls, 0);

  Data(Qt::DisplayRole);
  EXPECT_EQ(model_->get_cell_calls, 1);
}

}  // namespace
}  // namespace scada::aui
