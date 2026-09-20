// Menu-popup captures (the `auto-menu` manifest tag).

#include "publish_guard.h"
#include "screenshot_config.h"
#include "screenshot_fixture.h"
#include "screenshot_output.h"
#include "screenshot_wait.h"
#include "view_capture.h"
#include "widget_capture.h"

#include "app/client_application.h"
#include "aui/translation.h"
#include "controller/window_info.h"
#include "main_window/main_window.h"
#include "main_window/main_window_manager.h"
#include "main_window/opened_view/opened_view.h"
#include "profile/profile.h"
#include "profile/window_definition.h"

#include <QAction>
#include <QApplication>
#include <QComboBox>
#include <QMainWindow>
#include <QMenu>
#include <QMenuBar>
#include <QPixmap>
#include <QString>
#include <QToolBar>
#include <QToolButton>
#include <gtest/gtest.h>

#include <filesystem>
#include <string>

namespace {

using scada::screenshot_generator::FindTreeWidget;
using scada::screenshot_generator::FixtureConfig;
using scada::screenshot_generator::ScreenshotGenerator;
using scada::screenshot_generator::ShowMainWindowForMenuCapture;
using scada::screenshot_generator::WaitForAwaitable;
using scada::screenshot_generator::WaitForPendingNodeLoads;
using scada::screenshot_generator::WaitUntil;

}  // namespace

// Menu-popup captures (the `auto-menu` manifest tag).
//
// A `QMenu` renders offscreen without ever being popped up, so these grab the
// menu widget directly rather than driving a click. What they cannot skip is
// populating it: the client's menu bar is model-driven and `BuildMenu` refills
// each menu from its `MenuModel` on every `aboutToShow`, so a menu that has
// never been shown is empty and would still save a plausible-looking image.
// Emitting the signal is exactly what a real click does.
//
// These carry no `screenshot_data.json` spec — they drive the live widget tree
// rather than a fixture view — so `check_screenshots.py` has no dimensions to
// verify for them, and the content assertions below are the whole regression
// net. See "auto-menu — a menu popup" in docs/ops/client-screenshots.md.
namespace {

// Finds the menu-bar menu titled `Translate(english_title)` and populates it.
// Looks the title up through `Translate` rather than by its Russian text: the
// titles have been renamed since the manual's originals were captured (the
// menus the docs call «Далее» and «Схема» now read «Дополнительно» and
// «Мнемосхема»), and the English source string is the stable key.
QMenu* PopulateMenuBarMenu(QMainWindow* qmain, const char* english_title) {
  QMenuBar* menu_bar = qmain->menuBar();
  if (!menu_bar) {
    ADD_FAILURE() << "the main window has no menu bar";
    return nullptr;
  }
  const auto title = QString::fromStdU16String(Translate(english_title));
  QMenu* menu = nullptr;
  for (QAction* action : menu_bar->actions()) {
    if (action->menu() && action->text() == title)
      menu = action->menu();
  }
  if (!menu) {
    ADD_FAILURE() << "no " << english_title << " menu in the menu bar";
    return nullptr;
  }
  emit menu->aboutToShow();
  for (int i = 0; i < 10; ++i)
    QApplication::processEvents();
  return menu;
}

// Counts the menu's real rows — separators are `QAction`s too, and a menu of
// nothing but separators would otherwise read as populated.
int CountMenuRows(const QMenu& menu) {
  int rows = 0;
  for (const QAction* action : menu.actions()) {
    if (!action->isSeparator())
      ++rows;
  }
  return rows;
}

// Lays the populated menu out at its natural size and writes it to
// `OutputPathFor(filename)`.
void SaveMenuCapture(QMenu* menu,
                     const char* filename,
                     const CapturePublishGuard& publish_guard) {
  menu->ensurePolished();
  menu->adjustSize();
  for (int i = 0; i < 10; ++i)
    QApplication::processEvents();

  const auto output_dir = GetOutputDir();
  std::filesystem::create_directories(output_dir);

  if (!publish_guard.ShouldPublish())
    return;

  const QPixmap pixmap = GrabWhenSettled(menu);
  ASSERT_FALSE(pixmap.isNull()) << filename << " grabbed an empty pixmap";
  ASSERT_TRUE(
      pixmap.save(QString::fromStdString(OutputPathFor(filename).string())))
      << "could not write " << filename;
}

}  // namespace

// The More menu, whose subject in the manual is the "Export configuration to
// Excel..." row it ends with (dev/excel.md). The rows above it are the
// checkable view toggles, so the capture guards both: the export row must be
// present — it is `admin_only`, so a non-administrator fixture silently drops
// it — and the toggles above it must still be checkable.
TEST_F(ScreenshotGenerator, CaptureMoreMenu) {
  constexpr const char* kFilename = "menu-excel.png";
  if (!ShouldCaptureScreenshot(kFilename))
    GTEST_SKIP() << kFilename << " not requested";

  MainWindow::SetHideForTesting(false);

  // The manual shows this menu open over the object tree, and the view-scoped
  // rows resolve against whatever view is active, so boot the same page.
  {
    Profile profile;
    Page page;
    page.AddWindow(WindowDefinition{"Struct"});
    profile.AddPage(page);
    profile.Save();
  }

  WaitForAwaitable(executor_, app_.Start());
  ASSERT_TRUE(WaitForPendingNodeLoads(executor_, app_.node_service()));
  CapturePublishGuard publish_guard{kFilename};

  QMainWindow* qmain = ShowMainWindowForMenuCapture(app_);
  ASSERT_NE(qmain, nullptr);

  QMenu* menu = PopulateMenuBarMenu(qmain, "More");
  ASSERT_NE(menu, nullptr);
  EXPECT_GT(CountMenuRows(*menu), 1) << "the More menu rendered empty";

  const auto excel_title =
      QString::fromStdU16String(Translate("Export Configuration to Excel..."));
  int checkable_rows = 0;
  bool has_excel_row = false;
  for (const QAction* action : menu->actions()) {
    if (action->isSeparator())
      continue;
    if (action->text() == excel_title)
      has_excel_row = true;
    if (action->isCheckable())
      ++checkable_rows;
  }
  EXPECT_TRUE(has_excel_row)
      << "the More menu has no " << excel_title.toStdString()
      << " row - it is admin_only, so this capture is running without the "
         "administrator identity the manual's image depicts";
  EXPECT_GT(checkable_rows, 0) << "no checkable view rows above the export row";

  SaveMenuCapture(menu, kFilename, publish_guard);
}

// The Summary view's Function dropdown: the seven aggregate functions, one of
// them checked. This is a toolbar menu rather than a menu-bar one — the
// command toolbar groups the categories it cannot expand behind a
// `QToolButton` (see `MainWindow::CreateToolBar`) — but it populates the same
// way, through its own `aboutToShow`.
//
// Which function is checked comes from the fixture's Summ window, which sets
// no `AggregateType` and so takes the model's default. The manual's original
// shows Minimum; rather than pin a value the fixture does not state, this
// asserts the invariant that matters — the seven rows are all there and
// exactly one of them is checked.
TEST_F(ScreenshotGenerator, CaptureSummaryFunctionMenu) {
  constexpr const char* kFilename = "menu-summary.png";
  if (!ShouldCaptureScreenshot(kFilename))
    GTEST_SKIP() << kFilename << " not requested";

  MainWindow::SetHideForTesting(false);

  // The Function button's rows are the Summary view's own commands, so the
  // menu only resolves its checked state with a Summary view active.
  {
    Profile profile;
    Page page;
    page.AddWindow(WindowDefinition{"Summ"});
    profile.AddPage(page);
    profile.Save();
  }

  WaitForAwaitable(executor_, app_.Start());
  ASSERT_TRUE(WaitForPendingNodeLoads(executor_, app_.node_service()));

  QMainWindow* qmain = ShowMainWindowForMenuCapture(app_);
  ASSERT_NE(qmain, nullptr);

  // Activate the Summary view explicitly. The toolbar resolves every row
  // through `ResolveViewCommand`, which asks the *active* view — and a window
  // shown offscreen never receives the focus that would make one active, so
  // without this the seven rows exist but resolve no handler, and
  // `UpdateAction` hides them all. The menu still renders: seven invisible rows
  // grab as an empty popup.
  auto& main_window = app_.main_window_manager().main_windows().front();
  OpenedView* summary_view = nullptr;
  for (OpenedView* view : main_window.opened_views()) {
    if (view->window_info().name == std::string_view{"Summ"})
      summary_view = view;
  }
  ASSERT_NE(summary_view, nullptr) << "the Summary view did not open";
  main_window.ActivateView(*summary_view);
  for (int i = 0; i < 10; ++i)
    QApplication::processEvents();

  auto* toolbar = qmain->findChild<QToolBar*>("CommandToolbar");
  ASSERT_NE(toolbar, nullptr) << "no command toolbar in the main window";

  const auto function_title = QString::fromStdU16String(Translate("Function"));
  QMenu* menu = nullptr;
  for (QToolButton* button : toolbar->findChildren<QToolButton*>()) {
    if (button->text() == function_title && button->menu())
      menu = button->menu();
  }
  ASSERT_NE(menu, nullptr)
      << "no " << function_title.toStdString()
      << " tool button on the command toolbar - the aggregate commands are no "
         "longer grouped behind one";

  emit menu->aboutToShow();
  for (int i = 0; i < 10; ++i)
    QApplication::processEvents();

  CapturePublishGuard publish_guard{kFilename};

  // Seven aggregate functions: First, Last, Count, Minimum, Maximum, Sum,
  // Average (see RegisterSummaryCommandActions). A row that stopped being
  // checkable, or a menu that resolved no checked row, would still render.
  int visible_rows = 0;
  int checkable_rows = 0;
  int checked_rows = 0;
  for (const QAction* action : menu->actions()) {
    if (action->isSeparator())
      continue;
    if (action->isVisible())
      ++visible_rows;
    if (action->isCheckable())
      ++checkable_rows;
    if (action->isChecked())
      ++checked_rows;
  }
  EXPECT_EQ(CountMenuRows(*menu), 7) << "expected the seven aggregate rows";
  EXPECT_EQ(visible_rows, 7)
      << "an aggregate row resolved no command handler and was hidden";
  EXPECT_EQ(checkable_rows, 7) << "an aggregate row stopped being checkable";
  EXPECT_EQ(checked_rows, 1)
      << "expected exactly one aggregate function to read as selected";

  SaveMenuCapture(menu, kFilename, publish_guard);
}
