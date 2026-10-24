#include "aui/models/grid_model.h"
#include "aui/models/header_model.h"
#include "aui/models/table_model.h"
#include "aui/qt/grid.h"
#include "aui/qt/table.h"
#include "aui/test/app_environment.h"

#include <QShortcut>
#include <QTableView>
#include <QTest>
#include <QVBoxLayout>
#include <QWidget>

#include <gtest/gtest.h>

#include <memory>

namespace scada::aui {
namespace {

class OneCellGridModel : public GridModel {
 public:
  virtual void GetCell(GridCell& cell) override { cell.text = u"42"; }
};

class OneRowModel : public HeaderModel {
 public:
  virtual int GetCount() const override { return 1; }
  virtual int GetSize(int index) const override { return 17; }
  virtual std::u16string GetTitle(int index) const override { return {}; }
};

class OneRowTableModel : public TableModel {
 public:
  virtual int GetRowCount() override { return 1; }
  virtual void GetCell(TableCell& cell) override { cell.text = u"42"; }
};

// The client binds Ctrl+C window-wide to the selection's Copy command. A grid
// or table copies its own cells on the same key, and keeps it only because it
// claims the key's ShortcutOverride — these tests deliver a real key press
// through Qt's shortcut map, with a competing window shortcut, to pin that.
class CopyKeyOverrideTest : public testing::Test {
 protected:
  // Puts `view` in an active window beside a window-wide Ctrl+C shortcut, gives
  // it focus, presses Ctrl+C on it, and returns how often the shortcut fired.
  int PressCopyOn(QWidget& view) {
    QWidget window;
    auto* layout = new QVBoxLayout(&window);
    layout->addWidget(&view);

    int activations = 0;
    auto* shortcut = new QShortcut(QKeySequence::Copy, &window);
    shortcut->setContext(Qt::WindowShortcut);
    QObject::connect(shortcut, &QShortcut::activated,
                     [&activations] { ++activations; });

    window.show();
    window.activateWindow();
    EXPECT_TRUE(QTest::qWaitForWindowActive(&window));
    view.setFocus();
    QTest::keySequence(&view, QKeySequence::Copy);

    // The view belongs to the caller; take it back before the window dies.
    view.setParent(nullptr);
    return activations;
  }

  AppEnvironment app_env_;
};

// The control: a view that does not claim the key loses it to the shortcut.
// Without this the two assertions below would pass for a test that delivered
// no key at all.
TEST_F(CopyKeyOverrideTest, APlainViewLosesCopyToAWindowShortcut) {
  QTableView view;
  EXPECT_EQ(PressCopyOn(view), 1);
}

TEST_F(CopyKeyOverrideTest, GridKeepsCopy) {
  Grid grid{std::make_shared<OneCellGridModel>(),
            std::make_shared<OneRowModel>(),
            std::make_shared<ColumnHeaderModel>()};
  EXPECT_EQ(PressCopyOn(grid), 0);
}

TEST_F(CopyKeyOverrideTest, TableKeepsCopy) {
  Table table{std::make_shared<OneRowTableModel>(),
              {{0, u"Value", 100, TableColumn::LEFT}}};
  EXPECT_EQ(PressCopyOn(table), 0);
}

}  // namespace
}  // namespace scada::aui
