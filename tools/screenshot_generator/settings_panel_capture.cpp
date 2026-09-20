// The Settings surface capture, and with it the operator-facing Colour scheme
// choice.

#include "publish_guard.h"
#include "screenshot_config.h"
#include "screenshot_fixture.h"
#include "screenshot_output.h"
#include "screenshot_wait.h"
#include "widget_capture.h"

#include "app/client_application.h"
#include "aui/translation.h"
#include "main_window/main_window.h"
#include "main_window/main_window_manager.h"
#include "settings/qt/settings_panel.h"

#include <QAction>
#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QListWidget>
#include <QMainWindow>
#include <QMenu>
#include <QMenuBar>
#include <QPixmap>
#include <QString>
#include <gtest/gtest.h>

#include <algorithm>
#include <filesystem>
#include <string>

namespace {

using scada::screenshot_generator::FixtureConfig;
using scada::screenshot_generator::ScreenshotGenerator;
using scada::screenshot_generator::WaitForAwaitable;
using scada::screenshot_generator::WaitForPendingNodeLoads;

}  // namespace

// The Settings surface, and with it the operator-facing Colour scheme choice.
//
// It grabs the panel the shell builds rather than a form assembled here, for
// the reason the dialog capture did before it: a capture that re-derived the
// preferences would document a surface the client does not ship. The panel is
// an overlay over the main window and takes its rows from the same
// `MainMenuId::Settings` model the menu is built from, so what is grabbed is
// what the operator sees.
//
// The filename still says `dialog`. The surface stopped being one on
// 2026-08-28 and the manual pages that embed this image still describe the
// dialog, so renaming the file here would break those pages before anything
// could fix them — see the manifest note and backlog 628.
TEST_F(ScreenshotGenerator, CaptureSettingsPanel) {
  // The filename comes from the fixture rather than a literal, the way
  // CaptureDisplay already does it. Two reasons, and the second is the one that
  // forced it: a spec row is what `_fixture_captures` in the parity checker
  // reads, so a capture with no row can carry no `parity_key` and the matrix
  // cannot pair it with the web's — which is why the `settings` row sat in the
  // D22 worklist while a perfectly current picture of the surface was already
  // tracked. And it puts the rename that backlog 628 still owes in one place.
  const ScreenshotSpec* settings_spec = nullptr;
  for (const auto& spec : FixtureConfig().screenshots) {
    if (spec.capture == "settings") {
      settings_spec = &spec;
      break;
    }
  }
  ASSERT_NE(settings_spec, nullptr)
      << "no `settings` capture row in screenshot_data.json";
  const std::string kFilename = settings_spec->filename;
  if (!ShouldCaptureScreenshot(kFilename))
    GTEST_SKIP() << kFilename << " not requested";

  MainWindow::SetHideForTesting(false);

  const auto output_dir = GetOutputDir();
  std::filesystem::create_directories(output_dir);
  CapturePublishGuard publish_guard{kFilename};

  WaitForAwaitable(executor_, app_.Start());
  ASSERT_TRUE(WaitForPendingNodeLoads(executor_, app_.node_service()));
  for (int i = 0; i < 20; ++i)
    QApplication::processEvents();

  const auto& main_windows = app_.main_window_manager().main_windows();
  ASSERT_EQ(main_windows.size(), 1u);
  auto* qmain = dynamic_cast<QMainWindow*>(&main_windows.front());
  ASSERT_NE(qmain, nullptr);
  qmain->show();

  // The menu now carries one item that opens the surface, so the reachability
  // this used to guard on the Colour scheme submenu is guarded here instead:
  // Colour scheme is how the operator picks an appearance, and it must not
  // become unreachable.
  QMenuBar* menu_bar = qmain->menuBar();
  ASSERT_NE(menu_bar, nullptr);
  QMenu* settings_menu = nullptr;
  const auto settings_title = QString::fromStdU16String(Translate("Settings"));
  for (QAction* action : menu_bar->actions()) {
    if (action->menu() && action->text() == settings_title)
      settings_menu = action->menu();
  }
  ASSERT_NE(settings_menu, nullptr) << "no Settings menu in the menu bar";
  emit settings_menu->aboutToShow();
  for (int i = 0; i < 10; ++i)
    QApplication::processEvents();

  QAction* open_settings = nullptr;
  const auto item_title = QString::fromStdU16String(Translate("Settings..."));
  for (QAction* action : settings_menu->actions()) {
    if (!action->isSeparator() && action->text() == item_title)
      open_settings = action;
  }
  ASSERT_NE(open_settings, nullptr)
      << "Settings has no Settings... item - the preferences surface, and with "
         "it the appearance switch, is unreachable";

  // Opened the way the menu item and the rail's pinned utility both open it,
  // rather than by constructing a panel here.
  auto* main_window = dynamic_cast<MainWindow*>(&main_windows.front());
  ASSERT_NE(main_window, nullptr);
  main_window->ShowSettings();
  for (int i = 0; i < 10; ++i)
    QApplication::processEvents();

  auto* panel = qmain->findChild<SettingsPanel*>();
  ASSERT_NE(panel, nullptr) << "ShowSettings did not build the panel";
  EXPECT_FALSE(panel->isHidden());

  // Guard the contents, not just that a panel exists: an empty surface would
  // still render a plausible-looking image.
  const QList<QComboBox*> combos = panel->findChildren<QComboBox*>();
  const QList<QCheckBox*> checks = panel->findChildren<QCheckBox*>();
  EXPECT_GE(combos.size(), 1) << "expected Language / Style / Colour scheme";
  EXPECT_GT(checks.size(), 0) << "expected the preference toggles";
  bool has_appearances = false;
  for (const QComboBox* combo : combos) {
    if (combo->count() == 4)
      has_appearances = true;
  }
  EXPECT_TRUE(has_appearances) << "no row offers the four appearances";

  // The three things that make this a surface rather than the dialog it
  // replaced, and that no other capture in the gallery shows.
  //
  // These also carry weight beyond the image: this is the only place the
  // settings catalogue is exercised against the real `MainMenuModel` and the
  // real registered commands. `client_settings_qt_unittests` drives a
  // `SimpleMenuModel` over a fake delegate, and a fake there was once found
  // reporting a radio group as independent toggles -- a failure no test built
  // on that fake could distinguish from a real one. Measured 2026-08-28: with
  // `BuildSettingsCatalog` stubbed to return nothing, this test and 25 unit
  // tests fail and nothing else in the client suite notices.
  EXPECT_NE(panel->search_field(), nullptr);
  EXPECT_GT(panel->category_list()->count(), 1)
      << "the table of contents lists no categories";
  EXPECT_GT(panel->scope_tabs()->count(), 1)
      << "expected All plus a tab per storage scope";

  // Backlog 554: `Open Displays Folder` is an action the preferences dialog
  // had to skip, and the Displays category on this surface is where it landed.
  // A capture that stopped showing it would be that entry silently reopening.
  const bool has_displays_action =
      std::ranges::any_of(panel->visible_rows(), [](const SettingRow& row) {
        return row.control == SettingControl::kAction;
      });
  EXPECT_TRUE(has_displays_action) << "no action row on the surface";

  // The Language row must agree with the labels around it. It reads the locale
  // back from QSettings, which the fixture pins; without that pin it fell
  // through to the host's system locale and the published image read
  // "Язык: English" beside a form of Russian labels.
  bool language_matches_labels = false;
  const auto russian = QString::fromStdU16String(Translate("Russian"));
  for (const QComboBox* combo : combos) {
    if (combo->currentText() == russian)
      language_matches_labels = true;
  }
  EXPECT_TRUE(language_matches_labels)
      << "Language does not read " << russian.toStdString()
      << " - the capture is showing the host machine's locale, so this image "
         "renders differently depending on who generates it";

  // **A capture that failed its content checks must not publish an image.**
  // Every guard above is `EXPECT` rather than `ASSERT` on purpose, so one bad
  // render reports all of its problems at once instead of stopping at the
  // first — but `EXPECT` does not stop the test, so without this the save ran
  // anyway and a regeneration overwrote the tracked gallery PNG with the very
  // render the assertions had just rejected. Red test, bad file, and the file
  // is what gets committed.
  //
  // An empty settings surface is the case that makes this matter: it lays out
  // perfectly, so nothing about the image looks wrong (`capture.mjs` in the web
  // generator says the same thing about its own empty states, and backlog 583
  // is the Qt instance).
  // What was measured, and what follows rather than being measured: with the
  // catalogue emptied, `client_screenshot_check` fails and writes no
  // `settings-dialog.png` into the build directory it is given. That the
  // *tracked gallery* is likewise protected follows from `--out` being a
  // parameter -- the check passes a build path, `regenerate_client_screenshots`
  // passes `client/screenshots` -- and not from a run, because running
  // regeneration to find out would rewrite ~68 tracked PNGs as macOS renders in
  // a checkout other sessions are working in. Stated this way because the
  // commit that added the guard said "the tracked gallery was untouched
  // throughout", which was true of a run that could not have touched it either
  // way and so evidenced nothing.
  if (!publish_guard.ShouldPublish())
    return;

  QPixmap panel_pixmap = GrabWhenSettled(panel);
  ASSERT_FALSE(panel_pixmap.isNull());
  panel_pixmap.save(QString::fromStdString((OutputPathFor(kFilename)).string()));
}
