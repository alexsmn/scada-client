// The generator's three sweeps: every window on the fixture page, the
// standalone substation display, and every dialog.
//
// The other capture passes live beside this file — workbench_capture.cpp,
// settings_panel_capture.cpp, menu_capture.cpp — and register themselves with
// the same ScreenshotGenerator fixture.

#include "dialog_capture.h"
#include "display_capture.h"
#include "fixture_builder.h"
#include "screenshot_config.h"
#include "screenshot_fixture.h"
#include "screenshot_modules.h"
#include "screenshot_output.h"
#include "screenshot_wait.h"
#include "standalone_captures.h"
#include "view_capture.h"

#include "app/client_application.h"
#include "main_window/main_window.h"
#include "main_window/main_window_manager.h"
#include "main_window/opened_view/opened_view.h"
#include "profile/profile.h"
#include "profile/window_definition.h"

#include <QApplication>
#include <gtest/gtest.h>

#include <filesystem>
#include <iostream>
#include <set>
#include <string>

namespace {

using scada::screenshot_generator::CaptureViewSpec;
using scada::screenshot_generator::FindStandaloneCapture;
using scada::screenshot_generator::FixtureConfig;
using scada::screenshot_generator::ScreenshotGenerator;
using scada::screenshot_generator::SeedFavourites;
using scada::screenshot_generator::SeedLocalEvents;
using scada::screenshot_generator::StandaloneCapture;
using scada::screenshot_generator::StandaloneCaptureContext;
using scada::screenshot_generator::ViewCaptureContext;
using scada::screenshot_generator::WaitForAwaitable;
using scada::screenshot_generator::WaitForPendingNodeLoads;

}  // namespace

TEST_F(ScreenshotGenerator, CaptureAllWindows) {
  auto output_dir = GetOutputDir();
  std::filesystem::create_directories(output_dir);

  {
    Profile profile;
    // Keep screenshot windows stable: the fixture currently seeds only
    // historical events, so the normal auto-hide policy would close the
    // Event pane during startup before CaptureAllWindows inspects it.
    profile.event_auto_show = false;
    profile.event_auto_hide = false;
    // Portfolios reach the pane through the PROFILE rather than through a
    // seeder of ours: PortfolioModule's constructor calls
    // LoadPortfolios(profile_.data(), ...), so the production loader is the one
    // that runs and the fixture cannot drift from the shape the client reads.
    // That is also why this happens before Save() and before Start(), unlike
    // SeedFavourites below -- the favourites store is built during post-login,
    // the portfolio manager during module construction.
    if (const auto* portfolios =
            FixtureConfig().json.as_object().if_contains("portfolios")) {
      profile.data().as_object()["portfolios"] = *portfolios;
    }
    profile.AddPage(
        MakeScreenshotPage(FixtureConfig().screenshots, FixtureConfig().json));
    profile.Save();
  }

  WaitForAwaitable(executor_, app_.Start());

  // After Start, because the favourites store is built during post-login. The
  // pane's model subscribes to additions, so rows added now still reach it.
  SeedFavourites(FixtureConfig().json, app_.favourites());
  SeedLocalEvents(FixtureConfig().json, app_.local_events());

  // Wait for the data itself rather than pumping for a fixed second and
  // hoping: with many windows open that second was split too many ways, and a
  // view could be grabbed before its trends arrived.
  ASSERT_TRUE(scada::screenshot_generator::WaitForPendingData(
      executor_, app_.node_service(), app_.timed_data_service()));

  const auto& main_windows = app_.main_window_manager().main_windows();
  ASSERT_EQ(main_windows.size(), 1u);
  // Non-const: capturing a sidebar pane means selecting its activity-rail mode
  // first, the same way an operator would.
  MainWindow& main_window = const_cast<MainWindow&>(main_windows.front());

  int captured = 0;
  // Each non-standalone spec adds its own window to the fixture page (see
  // MakeScreenshotPage), so several specs can share a window_type — e.g. the
  // config-workbench "NewProps" specs (config-parameters, config-address-map on
  // TS.702; config-limits on TS.114). Consume the matching opened views in
  // page order so the Nth spec of a type gets the Nth view, instead of every
  // spec re-grabbing the first one (which pointed config-limits at TS.702's
  // form, whose Limits subtab does not exist).
  std::set<const OpenedView*> used_views;
  for (const auto& spec : FixtureConfig().screenshots) {
    // Standalone captures dispatch on `capture` — the screenshots-side twin of
    // `DialogSpec::kind`. Each builds its own fixture instead of grabbing an
    // opened view, and `MakeScreenshotPage` keeps every one of them off the
    // profile page.
    if (!spec.capture.empty()) {
      const StandaloneCapture* entry = FindStandaloneCapture(spec.capture);
      if (!entry) {
        ADD_FAILURE() << spec.filename << ": unknown capture: " << spec.capture;
        continue;
      }
      // A null handler is a spec another TEST_F renders; skip it without
      // counting it, which is what the old `display` arm's bare `continue`
      // did.
      if (!entry->save)
        continue;
      entry->save(StandaloneCaptureContext{
          .spec = spec,
          .main_window = main_window,
          .node_service = app_.node_service(),
          .timed_data_service = app_.timed_data_service(),
          .authenticated_attribute_service = authenticated_attribute_service_,
          .json = FixtureConfig().json,
          .executor = executor_,
      });
      ++captured;
      continue;
    }

    if (!CaptureViewSpec(spec,
                         ViewCaptureContext{
                             .main_window = main_window,
                             .executor = executor_,
                             .node_service = app_.node_service(),
                             .timed_data_service = app_.timed_data_service(),
                             .json = FixtureConfig().json,
                         },
                         used_views)) {
      continue;
    }
    ++captured;
  }

  std::cout << "Captured " << captured << "/"
            << FixtureConfig().screenshots.size() << " screenshots to "
            << output_dir.string() << std::endl;
}

TEST_F(ScreenshotGenerator, CaptureDisplay) {
  // The reshelled substation display renders standalone from a VDS fixture — it
  // isn't part of the profile page, so it can't be picked up by
  // CaptureAllWindows' view-matching loop. Cross-platform: the VDS renderer
  // paints without the Windows-only Modus/Vidicon ActiveX host.
  const ScreenshotSpec* display_spec = nullptr;
  for (const auto& spec : FixtureConfig().screenshots) {
    if (spec.capture == "display") {
      display_spec = &spec;
      break;
    }
  }
  if (!display_spec || !ShouldCaptureScreenshot(display_spec->filename))
    GTEST_SKIP() << "Display capture not requested";

  // The bay strips need the live services, so the app runs for this capture
  // exactly as it does for the view captures.
  WaitForAwaitable(executor_, app_.Start());
  ASSERT_TRUE(WaitForPendingNodeLoads(executor_, app_.node_service()));

  std::filesystem::create_directories(GetOutputDir());
  SaveDisplayScreenshot(*display_spec, executor_, FixtureConfig().json,
                        app_.timed_data_service(), app_.node_event_provider(),
                        app_.node_service());
}

TEST_F(ScreenshotGenerator, CaptureDialogs) {
  auto output_dir = GetOutputDir();
  std::filesystem::create_directories(output_dir);

  // Start the app so the real TimedDataServiceImpl is wired up; the
  // WriteDialog family reads current values, formula titles, and
  // engineering units through it.
  WaitForAwaitable(executor_, app_.Start());
  ASSERT_TRUE(WaitForPendingNodeLoads(executor_, app_.node_service()));
  for (int i = 0; i < 20; ++i)
    QApplication::processEvents();

  Profile profile;
  DialogEnvironment env{
      .executor = executor_,
      .node_service = &app_.node_service(),
      .timed_data_service = &app_.timed_data_service(),
      .profile = &profile,
      .favourites = &app_.favourites(),
      .dialog_analog_node_id = FixtureConfig().dialog_analog_node_id,
      .login_user_list = FixtureConfig().login_user_list};

  int captured = 0;
  for (const auto& spec : FixtureConfig().dialogs) {
    if (CaptureDialog(spec, env))
      ++captured;
  }

  std::cout << "Captured " << captured << "/" << FixtureConfig().dialogs.size()
            << " dialogs to " << output_dir.string() << std::endl;
}
