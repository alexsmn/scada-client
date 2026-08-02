#include "aui/qt/table_model_adapter.h"

#include "aui/models/table_model.h"
#include "aui/severity_colors.h"
#include "aui/test/app_environment.h"

#include <QFont>
#include <QVariant>
#include <gtest/gtest.h>

namespace scada::aui {
namespace {

// A minimal in-memory model: one row of fixed text, enough to drive the
// adapter's per-column roles.
class StubTableModel : public TableModel {
 public:
  virtual int GetRowCount() override { return 1; }
  virtual void GetCell(TableCell& cell) override { cell.text = u"42"; }
};

enum ColumnId { kTitleColumn, kValueColumn, kTimeColumn };

std::vector<TableColumn> MakeColumns() {
  return {
      {kTitleColumn, u"Title", 100, TableColumn::LEFT},
      {kValueColumn, u"Value", 100, TableColumn::RIGHT,
       TableColumn::DataType::General, /*monospace=*/true},
      {kTimeColumn, u"Time", 100, TableColumn::LEFT,
       TableColumn::DataType::DateTime},
  };
}

class TableModelAdapterTest : public testing::Test {
 protected:
  void TearDown() override { SetSeverityTheme(SeverityTheme::kLegacy); }

  QVariant FontFor(ColumnId column) {
    return adapter_.data(adapter_.index(0, column), Qt::FontRole);
  }

  int AlignmentFor(ColumnId column) {
    return adapter_.data(adapter_.index(0, column), Qt::TextAlignmentRole)
        .toInt();
  }

  AppEnvironment app_env_;
  TableModelAdapter adapter_{std::make_shared<StubTableModel>(), MakeColumns()};
};

// Under a token theme, value (monospace-flagged) and timestamp (Time)
// columns render in the fixed-pitch monospace value font so digits stay
// tabular as they update; other columns keep the default font.
TEST_F(TableModelAdapterTest, ValueAndTimestampColumnsRenderMonospace) {
  SetSeverityTheme(SeverityTheme::kDark);

  EXPECT_FALSE(FontFor(kTitleColumn).isValid());

  const QVariant value_font = FontFor(kValueColumn);
  ASSERT_TRUE(value_font.isValid());
  EXPECT_TRUE(value_font.value<QFont>().fixedPitch());

  const QVariant time_font = FontFor(kTimeColumn);
  ASSERT_TRUE(time_font.isValid());
  EXPECT_TRUE(time_font.value<QFont>().fixedPitch());
}

// The legacy look is unchanged: no column supplies a custom font.
TEST_F(TableModelAdapterTest, LegacyThemeKeepsTheDefaultFont) {
  for (ColumnId column : {kTitleColumn, kValueColumn, kTimeColumn})
    EXPECT_FALSE(FontFor(column).isValid());
}

// A column's alignment carries its vertical half too. Regression: the adapter
// returned the horizontal flag alone, leaving the vertical bits zero — which
// Qt reads as AlignTop, so every table row sat a pixel higher than the same
// row in a grid.
TEST_F(TableModelAdapterTest, ColumnAlignmentIsVerticallyCentred) {
  EXPECT_EQ(AlignmentFor(kTitleColumn),
            static_cast<int>(Qt::AlignLeft | Qt::AlignVCenter));
  EXPECT_EQ(AlignmentFor(kValueColumn),
            static_cast<int>(Qt::AlignRight | Qt::AlignVCenter));
}

}  // namespace
}  // namespace scada::aui
