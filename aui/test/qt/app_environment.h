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
// On macOS the platform defaults to `offscreen`. On cocoa every process that
// constructs a QApplication registers as a foreground application, so a
// `ctest` sweep -- one process per case, several at once under `-j` -- bounces
// a Dock icon and steals focus a few hundred times, and each widget a test
// shows is mapped on the real screen. Offscreen renders to memory: no Dock,
// no focus, no windows, and the same platform the screenshot generator and
// the client/server E2E already run the client on here. An explicit
// QT_QPA_PLATFORM still wins, so `QT_QPA_PLATFORM=cocoa` runs a test on the
// native platform when that is the point. The plugin itself has to be linked
// in for this to work with a static Qt, and the screen it presents has to be
// desktop-sized for a geometry-restoring test to get its window back
// unclamped: `scada_qt_import_offscreen_platform_into_tests()` in the
// client's root CMakeLists does the first for every test executable and
// passes the screen description's path as SCADA_QT_OFFSCREEN_PLATFORM_CONFIG
// for the second. **That definition is the only thing this header keys on.**
// It is set by the same CMake call that links the plugin, on the same
// target, so its presence proves the plugin is there; its absence -- a build
// tree configured before that call existed, or an export whose kit lacks it
// -- leaves the platform alone and the test runs on cocoa as it always did.
// Forcing `offscreen` on the definition's absence instead would abort every
// widget test at startup with `Could not find the Qt platform plugin
// "offscreen"`, in a tree that merely had not been reconfigured yet, which
// is what a peer session measured as 345 failures on 2026-09-07.
//
// The screen description is a source-tree path baked in at configure time,
// and the plugin refuses to start when the file is not there -- measured
// 2026-09-07: `Could not find platform config file <path>` and the process
// aborts. So the fixture looks before naming it: a binary run after its
// source tree moved falls back to the plugin's built-in 800x600 screen,
// still headless, rather than to that abort. Windows keeps its native
// platform, which is the one the client is styled for and the one the tests
// have always run on there.
class AppEnvironment {
 private:
  // A member so it runs before `app_`: QApplication reads QT_QPA_PLATFORM in
  // its constructor.
  struct PlatformDefault {
    PlatformDefault() {
#if defined(Q_OS_MACOS) && defined(SCADA_QT_OFFSCREEN_PLATFORM_CONFIG)
      if (qEnvironmentVariableIsEmpty("QT_QPA_PLATFORM")) {
        if (QFileInfo::exists(
                QStringLiteral(SCADA_QT_OFFSCREEN_PLATFORM_CONFIG))) {
          qputenv("QT_QPA_PLATFORM",
                  "offscreen:configfile=" SCADA_QT_OFFSCREEN_PLATFORM_CONFIG);
        } else {
          qputenv("QT_QPA_PLATFORM", "offscreen");
        }
      }
#endif
    }
  };
  [[no_unique_address]] PlatformDefault platform_default_;

  int argc_ = 0;
  QApplication app_{argc_, nullptr};

  // QApplication must be created.
  AnyExecutor executor_ = MakeAnyExecutor(std::make_shared<MessageLoopQt>());
};
