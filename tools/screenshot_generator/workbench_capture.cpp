// The three full-window workbench captures: the operator workbench hero, the
// Overview landing page, and the activity rail on its own.

#include "explorer_selection.h"
#include "publish_guard.h"
#include "screenshot_config.h"
#include "screenshot_fixture.h"
#include "screenshot_output.h"
#include "screenshot_wait.h"
#include "view_capture.h"
#include "widget_capture.h"

#include "app/client_application.h"
#include "aui/translation.h"
#include "aui/tree.h"
#include "controller/window_info.h"
#include "main_window/main_window.h"
#include "main_window/main_window_manager.h"
#include "main_window/opened_view/opened_view.h"
#include "profile/profile.h"
#include "profile/window_definition.h"

#include <QAction>
#include <QApplication>
#include <QCheckBox>
#include <QDockWidget>
#include <QLayout>
#include <QMainWindow>
#include <QPixmap>
#include <QToolBar>
#include <QToolButton>
#include <gtest/gtest.h>

#include <chrono>
#include <filesystem>
#include <set>
#include <string>
#include <utility>

namespace {

using scada::screenshot_generator::FindTreeWidget;
using scada::screenshot_generator::FixtureConfig;
using scada::screenshot_generator::MaterializeExplorerTree;
using scada::screenshot_generator::ScreenshotGenerator;
using scada::screenshot_generator::SelectSignalForInspector;
using scada::screenshot_generator::WaitForAwaitable;
using scada::screenshot_generator::WaitForPendingNodeLoads;
using scada::screenshot_generator::WaitUntil;

}  // namespace

TEST_F(ScreenshotGenerator, CaptureMainWindow) {
  // The operator workbench: activity rail, context bar with the severity
  // tiles, editor tabs, status strip. Saved under its own name so the
  // hand-maintained `client-window.png` — which shows a Modus display and stays
  // a manual capture until the fake Modus runtime exists — is never overwritten
  // by a generated render.
  constexpr const char* filename = "workbench-window.png";
  if (!ShouldCaptureScreenshot(filename))
    GTEST_SKIP() << filename << " not requested";

  MainWindow::SetHideForTesting(false);

  auto output_dir = GetOutputDir();
  std::filesystem::create_directories(output_dir);
  const auto output_image = OutputPathFor(filename);
  CapturePublishGuard publish_guard{filename};

  {
    Profile profile;
    Page page;
    page.AddWindow(WindowDefinition{"EventJournal"});
    page.AddWindow(WindowDefinition{"Summ"});
    page.AddWindow(WindowDefinition{"Struct"});
    profile.AddPage(page);
    profile.Save();
  }

  WaitForAwaitable(executor_, app_.Start());
  ASSERT_TRUE(WaitForPendingNodeLoads(executor_, app_.node_service()));

  for (int i = 0; i < 20; ++i)
    QApplication::processEvents();

  const auto& main_windows = app_.main_window_manager().main_windows();
  ASSERT_EQ(main_windows.size(), 1u);
  auto& main_window = main_windows.front();

  auto* qmain = dynamic_cast<QWidget*>(&main_window);
  ASSERT_NE(qmain, nullptr);
  qmain->resize(1920, 1080);
  qmain->ensurePolished();
  qmain->show();
  if (auto* central = qmain->layout())
    central->activate();
  if (auto* central_widget = qmain->findChild<QWidget*>())
    if (auto* layout = central_widget->layout())
      layout->activate();
  for (int i = 0; i < 20; ++i)
    QApplication::processEvents();

  // Verify the toolbar icon wiring against the live QWidget tree directly,
  // not the rendered pixels: the style must be icon-only, and the icon-bearing
  // command actions must have resolved their icons through LoadPixmap, which
  // returned empty pixmaps on macOS/Linux before the res/client.qrc fix. Not
  // every command carries an icon (many are image_id == 0, text-only), so we
  // assert that the pipeline produced icons, not that every action has one;
  // exhaustive per-id coverage lives in ClientUtilsQtTest.
  // Target the command toolbar specifically: the workbench chrome adds other
  // toolbars (activity rail, context bar) that carry no command icons.
  auto* toolbar = qmain->findChild<QToolBar*>("CommandToolbar");
  ASSERT_NE(toolbar, nullptr);
  EXPECT_EQ(toolbar->toolButtonStyle(), Qt::ToolButtonIconOnly);
  int actions_with_icons = 0;
  for (QAction* action : toolbar->actions()) {
    if (!action->isSeparator() && !action->icon().isNull())
      ++actions_with_icons;
  }
  EXPECT_GT(actions_with_icons, 0)
      << "no toolbar action resolved an icon (LoadPixmap/qrc regression)";

  scada::aui::Tree* tree = nullptr;
  QDockWidget* tree_dock = nullptr;
  // Kept for the selection below: the Inspector reads the *active* view's
  // selection, so the Explorer has to be activated as well as clicked in.
  OpenedView* struct_view = nullptr;
  for (OpenedView* view : main_window.opened_views()) {
    if (view->window_info().name != "Struct")
      continue;

    struct_view = view;
    tree = FindTreeWidget(view->view());
    tree_dock = qobject_cast<QDockWidget*>(view->view()->parentWidget());
    break;
  }
  ASSERT_NE(tree, nullptr);

  if (tree_dock) {
    tree_dock->show();
    tree_dock->raise();
    tree_dock->setMinimumWidth(420);
    main_window.resizeDocks({tree_dock}, {480}, Qt::Horizontal);
  }

  if (!MaterializeExplorerTree(*tree, tree_dock, executor_,
                               app_.node_service())) {
    MainWindow::SetHideForTesting(true);
    return;
  }

  ASSERT_NE(struct_view, nullptr);
  SelectSignalForInspector(main_window, *struct_view, *tree, executor_,
                           app_.node_service(), *qmain);

  for (int i = 0; i < 10; ++i)
    QApplication::processEvents();

  if (!publish_guard.ShouldPublish()) {
    MainWindow::SetHideForTesting(true);
    return;
  }

  QPixmap pixmap = GrabWhenSettled(qmain);
  pixmap.save(QString::fromStdString(output_image.string()));

  MainWindow::SetHideForTesting(true);
}

// The Overview landing cockpit: a fresh (page-less) profile boots through the
// production seeding path — BaseMainWindow falls back to CreateInitialPage,
// which returns MakeOverviewPage — so the capture guards the initial-page
// routing and the page's dominant-trend/alarm-strip split, not a
// hand-assembled page.
TEST_F(ScreenshotGenerator, CaptureOverviewPage) {
  const char* filename = "workbench-overview.png";
  if (!ShouldCaptureScreenshot(filename))
    GTEST_SKIP() << filename << " not requested";

  MainWindow::SetHideForTesting(false);

  auto output_dir = GetOutputDir();
  std::filesystem::create_directories(output_dir);
  const auto output_image = OutputPathFor(filename);
  CapturePublishGuard publish_guard{filename};

  // Deliberately no saved profile: the page-less boot is the state under test.
  WaitForAwaitable(executor_, app_.Start());
  ASSERT_TRUE(WaitForPendingNodeLoads(executor_, app_.node_service()));

  for (int i = 0; i < 20; ++i)
    QApplication::processEvents();

  const auto& main_windows = app_.main_window_manager().main_windows();
  ASSERT_EQ(main_windows.size(), 1u);
  auto& main_window = main_windows.front();

  // The seeded page carries the Overview pair plus the sidebar panes.
  std::set<std::string> view_names;
  for (OpenedView* view : main_window.opened_views())
    view_names.insert(std::string{view->window_info().name});
  EXPECT_TRUE(view_names.contains("Graph"));
  EXPECT_TRUE(view_names.contains("EventJournal"));

  // The sidebar panes really dock rather than opening as workspace tabs: each
  // is a WIN_SING pane, which the view manager routes into a dock widget and
  // tabifies with the others (the dock's tab bar is the pane switcher). This
  // is the runtime half of OverviewPageTest, which can only assert the page
  // composition - the window infos are registered by the running app.
  // The Objects mode's panes, which is what the rail selects by default.
  for (const char* pane : {"Struct", "Portfolio"}) {
    OpenedView* pane_view = nullptr;
    for (OpenedView* view : main_window.opened_views()) {
      if (view->window_info().name == pane) {
        pane_view = view;
        break;
      }
    }
    ASSERT_NE(pane_view, nullptr) << pane << " missing from the Overview page";
    EXPECT_TRUE(pane_view->window_info().is_pane()) << pane;
    ASSERT_NE(pane_view->view(), nullptr) << pane;
    EXPECT_NE(qobject_cast<QDockWidget*>(pane_view->view()->parentWidget()),
              nullptr)
        << pane << " is not docked - it would open as a workspace tab";
  }

  auto* qmain = dynamic_cast<QWidget*>(&main_window);
  ASSERT_NE(qmain, nullptr);

  // The alarm table honours the page's "Current" mode: the journal opens
  // scoped to actionable events — the unacknowledged-only filter pre-set
  // (regression: the mode item was written by every current-events open path
  // but consumed by nothing, so the Overview landed on the full history).
  auto* unacknowledged_only =
      qmain->findChild<QCheckBox*>(QStringLiteral("unacknowledgedOnly"));
  ASSERT_NE(unacknowledged_only, nullptr);
  EXPECT_TRUE(unacknowledged_only->isChecked());
  qmain->resize(1920, 1080);
  qmain->ensurePolished();
  qmain->show();
  scada::screenshot_generator::PumpEventLoopFor(std::chrono::milliseconds(500));

  // The landing's Inspector is a pane like any other, so with nothing selected
  // it renders its "select an item" placeholder — which published the whole
  // right-hand third of the hero as an empty column. Make the selection the
  // shell's own way, through the Explorer pane the page already docks, so the
  // capture exercises the selection→Inspector wiring rather than posing it.
  {
    scada::aui::Tree* tree = nullptr;
    OpenedView* struct_view = nullptr;
    for (OpenedView* view : main_window.opened_views()) {
      if (view->window_info().name != "Struct")
        continue;
      struct_view = view;
      tree = FindTreeWidget(view->view());
      break;
    }
    ASSERT_NE(struct_view, nullptr);
    ASSERT_NE(tree, nullptr);

    SelectSignalForInspector(main_window, *struct_view, *tree, executor_,
                             app_.node_service(), *qmain);
    for (int i = 0; i < 10; ++i)
      QApplication::processEvents();
  }

  if (!publish_guard.ShouldPublish()) {
    MainWindow::SetHideForTesting(true);
    return;
  }

  QPixmap pixmap = GrabWhenSettled(qmain);
  pixmap.save(QString::fromStdString(output_image.string()));

  MainWindow::SetHideForTesting(true);
}

// The activity rail on its own — the manual documents it as a surface in its
// own right, and a 1920px window shot cannot show a 52px column legibly.
//
// Captured with several pages so the middle band reads as a group rather than
// as one button, which is the whole point of the band.
TEST_F(ScreenshotGenerator, CaptureActivityRail) {
  constexpr const char* kFilename = "workbench-activity-rail.png";
  if (!ShouldCaptureScreenshot(kFilename))
    GTEST_SKIP() << kFilename << " not requested";

  MainWindow::SetHideForTesting(false);

  auto output_dir = GetOutputDir();
  std::filesystem::create_directories(output_dir);
  CapturePublishGuard publish_guard{kFilename};

  {
    Profile profile;
    // Three pages, each with an icon, so the band shows the operator's own
    // glyphs rather than a column of ordinals.
    for (const auto& [title, icon] :
         {std::pair{u"Overview", "overview"}, std::pair{u"Alarms", "alarms"},
          std::pair{u"Trends", "trend"}}) {
      Page page;
      page.title = title;
      page.icon = icon;
      page.AddWindow(WindowDefinition{"Struct"});
      profile.AddPage(page);
    }
    profile.Save();
  }

  WaitForAwaitable(executor_, app_.Start());
  ASSERT_TRUE(WaitForPendingNodeLoads(executor_, app_.node_service()));
  for (int i = 0; i < 20; ++i)
    QApplication::processEvents();

  const auto& main_windows = app_.main_window_manager().main_windows();
  ASSERT_EQ(main_windows.size(), 1u);
  auto* qmain = dynamic_cast<QWidget*>(&main_windows.front());
  ASSERT_NE(qmain, nullptr);
  qmain->resize(1920, 1080);
  qmain->show();
  for (int i = 0; i < 20; ++i)
    QApplication::processEvents();

  auto* rail = qmain->findChild<QWidget*>("activityBar");
  ASSERT_NE(rail, nullptr) << "the reshell rail is not in the window";

  // The three zones must all be present, or the image documents a rail that
  // is missing one and nothing would say so.
  const QList<QToolButton*> buttons = rail->findChildren<QToolButton*>();
  EXPECT_GE(buttons.size(), 3 + 3 + 1 + 1)
      << "expected pane modes, three pages, the '+' and at least one utility";

  if (!publish_guard.ShouldPublish())
    return;

  const QPixmap frame = GrabWhenSettled(rail);
  ASSERT_FALSE(frame.isNull());
  ASSERT_TRUE(
      frame.save(QString::fromStdString((OutputPathFor(kFilename)).string())))
      << "could not write " << kFilename;
}
