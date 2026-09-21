#pragma once

#include "base/any_executor.h"

#include "aui/qt/message_loop_qt.h"

#include <QApplication>
#include <QFileInfo>

// The application a widget test runs under: a QApplication plus the executor
// the client's Qt code posts to. One per fixture, as a member -- never a
// static in a test binary, which would be destroyed during atexit teardown
// where ~QGuiApplication crashes on macOS after other Qt statics are gone.
//
// The fixture makes two separate decisions about Qt's platform, and they are
// guarded differently: WHICH PLATFORM, which is a macOS question, and WHICH
// SCREEN, which is not.
//
// WHICH PLATFORM. On macOS the platform defaults to `offscreen`. On cocoa
// every process that constructs a QApplication registers as a foreground
// application, so a `ctest` sweep -- one process per case, several at once
// under `-j` -- bounces a Dock icon and steals focus a few hundred times, and
// each widget a test shows is mapped on the real screen. Offscreen renders to
// memory: no Dock, no focus, no windows, and the same platform the screenshot
// generator and the client/server E2E already run the client on here. An
// explicit QT_QPA_PLATFORM still wins, so `QT_QPA_PLATFORM=cocoa` runs a test
// on the native platform when that is the point. Windows keeps its native
// platform, which is the one the client is styled for and the one the tests
// have always run on there.
//
// The plugin itself has to be linked in for this to work with a static Qt:
// `scada_qt_import_offscreen_platform_into_tests()` in the client's root
// CMakeLists does that for every test executable in the build and passes the
// screen description's path as SCADA_QT_OFFSCREEN_PLATFORM_CONFIG.
// **That definition is the only thing this header keys on.** It is set by the
// same CMake call that links the plugin, on the same target, so its presence
// proves the plugin is there; its absence -- a build tree configured before
// that call existed, or an export whose kit lacks it -- leaves the platform
// alone and the test runs on cocoa as it always did. Forcing `offscreen` on
// the definition's absence instead would abort every widget test at startup
// with `Could not find the Qt platform plugin "offscreen"`, in a tree that
// merely had not been reconfigured yet, which is what a peer session measured
// as 345 failures on 2026-09-07.
//
// WHICH SCREEN, and this half runs on every platform. The offscreen plugin's
// built-in screen is 800x600, and `QWidget::restoreGeometry()` clamps a
// restored window to the screen it lands on, so a geometry-restoring test
// needs a desktop-sized one -- which is what `configfile=` names. A caller
// who sets `QT_QPA_PLATFORM=offscreen` by hand, the obvious thing on a
// headless runner, would otherwise win over that and silently bring the small
// screen with them, so an unadorned `offscreen` is upgraded to carry the
// screen description rather than being left alone. The upgrade is exact:
// `cocoa`, `minimal` and an `offscreen` that already carries options are
// untouched, so `QT_QPA_PLATFORM=offscreen:configfile=<your own json>` is how
// to ask for a different screen. Nothing in the client asserts on restored
// geometry today; four Designer tests do, and they are what measured this
// (backlog 807).
//
// The screen description is a source-tree path baked in at configure time,
// and the plugin refuses to start when the file is not there -- measured
// 2026-09-07: `Could not find platform config file <path>` and the process
// aborts. So the fixture looks before naming it, in both halves: a binary run
// after its source tree moved falls back to the plugin's built-in 800x600
// screen, still headless, rather than to that abort.
class AppEnvironment {
 private:
  // A member so it runs before `app_`: QApplication reads QT_QPA_PLATFORM in
  // its constructor.
  struct PlatformDefault {
    PlatformDefault() {
#if defined(SCADA_QT_OFFSCREEN_PLATFORM_CONFIG)
      const bool have_screen =
          QFileInfo::exists(QStringLiteral(SCADA_QT_OFFSCREEN_PLATFORM_CONFIG));
      static constexpr const char* kWithScreen =
          "offscreen:configfile=" SCADA_QT_OFFSCREEN_PLATFORM_CONFIG;

      // Which platform: macOS only, and only when the caller named none.
#if defined(Q_OS_MACOS)
      if (qEnvironmentVariableIsEmpty("QT_QPA_PLATFORM"))
        qputenv("QT_QPA_PLATFORM", have_screen ? kWithScreen : "offscreen");
#endif

      // Which screen: on every platform, and including the case the caller
      // asked for. An unadorned `offscreen` gets the screen description
      // attached; anything else -- `cocoa`, or an `offscreen` already
      // carrying options -- is left exactly as it was.
      if (have_screen && qgetenv("QT_QPA_PLATFORM") == "offscreen")
        qputenv("QT_QPA_PLATFORM", kWithScreen);
#endif
    }
  };
  [[no_unique_address]] PlatformDefault platform_default_;

  int argc_ = 0;
  QApplication app_{argc_, nullptr};

  // QApplication must be created.
  AnyExecutor executor_ = MakeAnyExecutor(std::make_shared<MessageLoopQt>());
};
