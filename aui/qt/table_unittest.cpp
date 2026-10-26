#include "aui/qt/table.h"

#include "aui/models/table_model.h"
#include "aui/qt/theme_qt.h"
#include "aui/severity_colors.h"
#include "aui/test/app_environment.h"

#include <QApplication>
#include <QFontMetrics>
#include <QGuiApplication>
#include <QHeaderView>

#include <gtest/gtest.h>

#include <vector>

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

// A model whose rows arrive after construction, which is how the node tables
// populate — and the case that makes a constructor-time content measurement
// wrong.
class LateTableModel : public TableModel {
 public:
  virtual int GetRowCount() override { return rows_; }
  virtual void GetCell(TableCell& cell) override {
    cell.text =
        u"SCADA.9401 \u0417\u0430\u043c\u043a\u043d\u0443\u0442/"
        u"\u0420\u0430\u0437\u043e\u043c\u043a\u043d\u0443\u0442";
  }
  // Notifies the way a real model does, so the adapter emits rowsInserted and
  // the view's deferred sizing runs — which is the behaviour under test.
  void Populate() {
    ScopedItemsAdding adding{*this, 0, 1};
    rows_ = 1;
  }

 private:
  int rows_ = 0;
};

class TableTest : public testing::Test {
 protected:
  void TearDown() override {
    // The palette and the severity ramp are application state; leaving an
    // explicitly themed one installed would silently change every later test in
    // this binary. Re-applying the default appearance resets both — this used
    // to be two calls, and after ApplyTheme took over the ramp the second one
    // undid the first.
    ApplyTheme(Theme::kDark);
  }

  AppEnvironment app_env_;
};

// The value/timestamp columns render in the monospace font, so their default
// widths widen with it when it is the wider face — a timestamp that fits the
// configured width in the UI font must not elide in monospace. Other columns
// keep their configured width.
//
// Whether the monospace face IS wider belongs to the host's fonts, not to
// Table, so the expectation is measured the way the code measures it rather
// than assumed. It is wider on macOS and Linux. The Windows CI runner once
// measured 170 vs 170 (scada-client run 40138625293), because offscreen Qt
// there had no fonts at all and both faces were the same fallback; the
// workflow now names C:\Windows\Fonts as QT_QPA_FONTDIR.
TEST_F(TableTest, MonospaceColumnDefaultsWidenWithTheFont) {
  const QString sample = QStringLiteral("00.00.0000 00:00:00.000");
  const bool mono_is_wider =
      QFontMetrics{MonoValueFont()}.horizontalAdvance(sample) >
      QFontMetrics{QGuiApplication::font()}.horizontalAdvance(sample);

  Table table{std::make_shared<StubTableModel>(), MakeColumns()};
  EXPECT_EQ(table.columnWidth(kTitleColumn), kConfiguredWidth);
  if (mono_is_wider) {
    EXPECT_GT(table.columnWidth(kValueColumn), kConfiguredWidth);
    EXPECT_GT(table.columnWidth(kTimeColumn), kConfiguredWidth);
  } else {
    EXPECT_EQ(table.columnWidth(kValueColumn), kConfiguredWidth);
    EXPECT_EQ(table.columnWidth(kTimeColumn), kConfiguredWidth);
  }
}

// Item views paint their interior from the *Window* colour
// (SetDefaultItemColors folds Window into Base/AlternateBase/Text) so a grid
// matches the chrome around it. That has to keep tracking the application
// palette after construction: a view built before the theme is applied — which
// is every view in a running client, since the operator can switch themes live
// — would otherwise keep painting the palette it was born with.
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

// Column visibility. The header right-click menu is the operator-facing form;
// this is the API under it.
TEST_F(TableTest, ColumnsCanBeHiddenAndShown) {
  Table table{std::make_shared<StubTableModel>(), MakeColumns()};

  EXPECT_TRUE(table.IsColumnVisible(kTimeColumn));
  table.SetColumnVisible(kTimeColumn, false);
  EXPECT_FALSE(table.IsColumnVisible(kTimeColumn));

  table.SetColumnVisible(kTimeColumn, true);
  EXPECT_TRUE(table.IsColumnVisible(kTimeColumn));
}

// A print or an export takes VisibleColumns(), so a hidden column must not be
// in it and the header's order must be kept (backlog 912).
TEST_F(TableTest, VisibleColumnsSkipHiddenOnesInHeaderOrder) {
  Table table{std::make_shared<StubTableModel>(), MakeColumns()};
  table.SetColumnVisible(kValueColumn, false);
  table.horizontalHeader()->moveSection(/*from=*/2, /*to=*/0);

  std::vector<int> ids;
  for (const TableColumn& column : table.VisibleColumns())
    ids.push_back(column.id);

  EXPECT_EQ(ids, (std::vector<int>{kTimeColumn, kTitleColumn}));
}

// Hiding the last one is refused: the header context menu is the only way to
// bring a column back, and a header with no sections has nothing to
// right-click — the table would be unrecoverable short of editing the profile.
TEST_F(TableTest, TheLastVisibleColumnCannotBeHidden) {
  Table table{std::make_shared<StubTableModel>(), MakeColumns()};

  table.SetColumnVisible(kValueColumn, false);
  table.SetColumnVisible(kTimeColumn, false);
  ASSERT_TRUE(table.IsColumnVisible(kTitleColumn));

  table.SetColumnVisible(kTitleColumn, false);

  EXPECT_TRUE(table.IsColumnVisible(kTitleColumn));
}

// A hidden column has to survive save/restore, or it comes back every time the
// view is reopened.
TEST_F(TableTest, HiddenColumnsSurviveSaveAndRestore) {
  Table saved{std::make_shared<StubTableModel>(), MakeColumns()};
  saved.SetColumnVisible(kValueColumn, false);

  Table restored{std::make_shared<StubTableModel>(), MakeColumns()};
  restored.RestoreState(saved.SaveState());

  EXPECT_FALSE(restored.IsColumnVisible(kValueColumn));
  EXPECT_TRUE(restored.IsColumnVisible(kTitleColumn));
  EXPECT_TRUE(restored.IsColumnVisible(kTimeColumn));
}

// A hidden section reports width 0. Restoring that verbatim would bring the
// column back as an ungrabbable sliver.
TEST_F(TableTest, AHiddenColumnRestoresWithAUsableWidth) {
  Table saved{std::make_shared<StubTableModel>(), MakeColumns()};
  saved.SetColumnVisible(kValueColumn, false);

  Table restored{std::make_shared<StubTableModel>(), MakeColumns()};
  restored.RestoreState(saved.SaveState());
  restored.SetColumnVisible(kValueColumn, true);

  EXPECT_TRUE(restored.IsColumnVisible(kValueColumn));
  EXPECT_GT(restored.columnWidth(kValueColumn), 0);
}

// V42: a `size_to_content` column takes its width from its content, not from
// the `width` field — which for a server-supplied name can only ever be a
// guess. The admin grids carried a hard 75px and truncated to `SCADA.94…`
// at every window size; widening the CAPTURE to 1000px changed nothing, which
// is what proved the image was never the constraint.
TEST_F(TableTest, SizeToContentColumnIgnoresItsDeclaredWidth) {
  auto model = std::make_shared<LateTableModel>();
  model->Populate();
  std::vector<TableColumn> columns = {
      {kTitleColumn, u"Title", /*width=*/10, TableColumn::LEFT,
       TableColumn::DataType::General, /*monospace=*/false,
       /*size_to_content=*/true}};
  Table table{model, columns};

  // 10px was the declared width and could not hold one character.
  EXPECT_GT(table.columnWidth(0), 10);
}

// The reason the sizing is deferred rather than done in the constructor: these
// tables are empty when they are built, so measuring then would size the
// column to its HEADER and leave the content truncated exactly as before.
TEST_F(TableTest, SizeToContentColumnIsMeasuredWhenTheRowsArrive) {
  auto model = std::make_shared<LateTableModel>();
  std::vector<TableColumn> columns = {
      {kTitleColumn, u"T", /*width=*/10, TableColumn::LEFT,
       TableColumn::DataType::General, /*monospace=*/false,
       /*size_to_content=*/true}};
  Table table{model, columns};
  const int empty_width = table.columnWidth(0);

  model->Populate();

  EXPECT_GT(table.columnWidth(0), empty_width)
      << "a column sized while the model was empty stays too narrow for the "
         "content that arrives later";
}

// A column without the flag keeps its declared width, so this cannot quietly
// re-size every table in the client — `table.png`'s nine hand-tuned columns
// and the manual-referenced `users.png` depend on that.
TEST_F(TableTest, ColumnsWithoutTheFlagKeepTheirDeclaredWidth) {
  auto model = std::make_shared<LateTableModel>();
  model->Populate();
  std::vector<TableColumn> columns = {
      {kTitleColumn, u"Title", kConfiguredWidth, TableColumn::LEFT}};
  Table table{model, columns};

  EXPECT_EQ(table.columnWidth(0), kConfiguredWidth);
}

// `ResizeToContents` as a MODE would have fixed the width too: it makes the
// section non-draggable, and an operator resizing a column is ordinary. The
// one-shot resize leaves the header interactive.
TEST_F(TableTest, SizeToContentLeavesTheSectionResizable) {
  auto model = std::make_shared<LateTableModel>();
  model->Populate();
  std::vector<TableColumn> columns = {
      {kTitleColumn, u"Title", /*width=*/10, TableColumn::LEFT,
       TableColumn::DataType::General, /*monospace=*/false,
       /*size_to_content=*/true}};
  Table table{model, columns};

  EXPECT_EQ(table.horizontalHeader()->sectionResizeMode(0),
            QHeaderView::Interactive);
}

}  // namespace
}  // namespace scada::aui
