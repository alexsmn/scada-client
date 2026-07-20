#include "aui/qt/table.h"

#include "aui/models/table_model.h"
#include "aui/severity_colors.h"
#include "aui/test/app_environment.h"

#include <gtest/gtest.h>

namespace scada::aui {
namespace {

// A minimal in-memory model: one row of fixed text.
class StubTableModel : public TableModel {
 public:
  virtual int GetRowCount() override { return 1; }
  virtual void GetCell(TableCell& cell) override { cell.text = u"42"; }
};

enum ColumnId { kTitleColumn, kValueColumn, kTimeColumn };

constexpr int kConfiguredWidth = 170;

std::vector<TableColumn> MakeColumns() {
  return {
      {kTitleColumn, u"Title", kConfiguredWidth, TableColumn::LEFT},
      {kValueColumn, u"Value", kConfiguredWidth, TableColumn::RIGHT,
       TableColumn::DataType::General, /*monospace=*/true},
      {kTimeColumn, u"Time", kConfiguredWidth, TableColumn::LEFT,
       TableColumn::DataType::DateTime},
  };
}

class TableTest : public testing::Test {
 protected:
  void TearDown() override { SetSeverityTheme(SeverityTheme::kLegacy); }

  AppEnvironment app_env_;
};

// The legacy look is unchanged: every column takes its configured width.
TEST_F(TableTest, LegacyThemeKeepsConfiguredColumnWidths) {
  Table table{std::make_shared<StubTableModel>(), MakeColumns()};
  EXPECT_EQ(table.columnWidth(kTitleColumn), kConfiguredWidth);
  EXPECT_EQ(table.columnWidth(kValueColumn), kConfiguredWidth);
  EXPECT_EQ(table.columnWidth(kTimeColumn), kConfiguredWidth);
}

// Under a token theme the value/timestamp columns render in the (wider)
// monospace font, so their default widths widen with it — a timestamp that
// fit the configured width in the UI font must not elide in monospace. Other
// columns keep their configured width.
TEST_F(TableTest, TokenThemeWidensMonospaceColumnDefaults) {
  SetSeverityTheme(SeverityTheme::kDark);
  Table table{std::make_shared<StubTableModel>(), MakeColumns()};
  EXPECT_EQ(table.columnWidth(kTitleColumn), kConfiguredWidth);
  EXPECT_GT(table.columnWidth(kValueColumn), kConfiguredWidth);
  EXPECT_GT(table.columnWidth(kTimeColumn), kConfiguredWidth);
}

}  // namespace
}  // namespace scada::aui
