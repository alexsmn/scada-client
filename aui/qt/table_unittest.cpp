#include "aui/qt/table.h"

#include "aui/models/table_model.h"
#include "aui/severity_colors.h"
#include "aui/qt/theme_qt.h"
#include "aui/test/app_environment.h"

#include <QApplication>

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
  void TearDown() override {
    SetSeverityTheme(SeverityTheme::kLegacy);
    // The palette is application state; leaving a themed one installed would
    // silently change every later test in this binary.
    ApplyTheme(Theme::kSystem, ThemeScope::kPaletteOnly);
  }

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


// Item views paint their interior from the *Window* colour (SetDefaultItemColors
// folds Window into Base/AlternateBase/Text) so a grid matches the chrome
// around it. That has to keep tracking the application palette after
// construction: a view built before the theme is applied — which is every view
// in a running client, since the operator can switch themes live — would
// otherwise keep painting the palette it was born with.
TEST_F(TableTest, FollowsALaterApplicationPaletteChange) {
  Table table{std::make_shared<StubTableModel>(), MakeColumns()};

  ApplyTheme(Theme::kDark, ThemeScope::kPaletteOnly);
  QApplication::processEvents();

  const QColor window = QApplication::palette().color(QPalette::Window);
  EXPECT_EQ(table.palette().color(QPalette::Base), window);
  EXPECT_EQ(table.palette().color(QPalette::AlternateBase), window);
  EXPECT_EQ(table.palette().color(QPalette::Text),
            QApplication::palette().color(QPalette::WindowText));

  // And back again, so the switch is not one-way.
  ApplyTheme(Theme::kLight, ThemeScope::kPaletteOnly);
  QApplication::processEvents();
  EXPECT_EQ(table.palette().color(QPalette::Base),
            QApplication::palette().color(QPalette::Window));
}

}  // namespace
}  // namespace scada::aui
