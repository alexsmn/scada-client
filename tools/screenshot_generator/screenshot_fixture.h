#pragma once

#include "screenshot_config.h"

#include "address_space/attribute_service_impl.h"
#include "address_space/local_history_service.h"
#include "address_space/local_method_service.h"
#include "address_space/local_monitored_item_service.h"
#include "address_space/local_node_management_service.h"
#include "address_space/local_session_service.h"
#include "address_space/test/scada_test_address_space.h"
#include "address_space/view_service_impl.h"
#include "app/client_application.h"
#include "aui/qt/message_loop_qt.h"
#include "aui/test/app_environment.h"
#include "authenticated_attribute_service.h"
#include "base/any_executor.h"
#include "base/client_paths.h"
#include "base/no_destructor.h"
#include "base/test/scoped_mock_clock_override.h"
#include "base/test/scoped_path_override.h"
#include "fixture_builder.h"
#include "node_service/node_service.h"
#include "scada/access_rights.h"
#include "screenshot_modules.h"
#include "screenshot_options.h"

#include <QSettings>
#include <QTemporaryDir>
#include <QTranslator>

#include <gtest/gtest.h>

#include <string>
#include <vector>

namespace boost::json {
class value;
}

class Favourites;
class LocalEvents;
class QMainWindow;

namespace scada::screenshot_generator {

// The fixture JSON, loaded once per test suite by
// `ScreenshotGenerator::SetUpTestSuite` and read by every capture and
// assertion. Shared rather than file-private because the fixture below and the
// tests that drive it no longer live in one translation unit.
ScreenshotConfig& FixtureConfig();

// Seeds the device-log lines the fixture's `device_log` block defines as event
// notifications on the device, so the Log capture renders a real session
// instead of an empty grid. A `frame` object makes the line arrive as a
// DeviceFrameEvent, which is what fills the decoded columns; without one it is
// a plain log line, the shape an older server sends.
void SeedDeviceLog(const boost::json::value& root,
                   scada::LocalMonitoredItemService& monitored_item_service);

// Seeds the fixture's `favourites` folders and their windows, so the
// Favourites pane renders a populated tree.
void SeedFavourites(const boost::json::value& root, Favourites& favourites);

// Reports the fixture's client-side events, which is the only way a
// `LOCAL_EVENT` row reaches the journal: nothing in an offline fixture loses a
// server connection, so the «Local Event» object the manual's event-journal
// pages show never appeared in a generated capture. Absent key: nothing
// reported, same as before.
void SeedLocalEvents(const boost::json::value& root, LocalEvents& local_events);

// Returns the fixture's single `MainWindow` as a `QMainWindow`, shown and
// pumped so its menu bar and command toolbar are live. `app.Start()` must
// already have run.
QMainWindow* ShowMainWindowForMenuCapture(ClientApplication& app);

// The fixture accounts that carry `right`, by display name — the same
// projection `fixture_builder.cpp` makes when it creates the identity mapping
// rules. Derived from the fixture JSON rather than written out, both because
// the names are Russian (which may not appear in a client string literal) and
// so the expectation cannot drift from the fixture it describes.
std::vector<std::u16string> FixtureAccountsWith(scada::AccessRight right);

class ScreenshotGenerator : public ::testing::Test {
 public:
  static void SetUpTestSuite() {
    // Hermetic settings: the client reads default-constructed QSettings
    // (registry on Windows, plists on macOS), so on a used dev box the
    // captures would inherit real state — e.g. the last-used server
    // address in the login dialog. Redirect the default format into a
    // scratch ini tree so every run renders from a factory-fresh profile
    // regardless of the machine.
    static scada::base::NoDestructor<QTemporaryDir> settings_dir;
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope,
                       settings_dir->path());
    QSettings::setPath(QSettings::IniFormat, QSettings::SystemScope,
                       settings_dir->path());
    QSettings::setDefaultFormat(QSettings::IniFormat);

    // Every dialog a capture grabs must be a Qt widget. QMessageBox and
    // QFileDialog hand themselves to the platform theme when one offers a
    // native implementation, and macOS does (QCocoaMessageDialog, since Qt
    // 6.2): the box then becomes an NSAlert, which is not a QDialog, not in
    // `QApplication::topLevelWidgets()`, and not reachable by `reject()`. On
    // the cocoa platform that turned the `control-confirm` and `message-box`
    // kinds — the two that go through `RunMessageBox` — into a capture the
    // grab could never find and a modal the run could never dismiss;
    // depending on timing the suite then hung inside
    // `QCocoaMessageDialog::show` or segfaulted on the next one, in both
    // cases never reaching the specs behind it. Both ctest nets run
    // offscreen, whose theme offers no native dialogs at all, so neither
    // could see it — only a bare run on a Mac reproduced it.
    //
    // Pinning the attribute is the same decision as pinning the Fusion style
    // below: a published capture must be a property of the fixture, not of
    // the host. Note this is deliberately NOT what the client does — there,
    // native dialogs are the desired end state (docs/client/ux/dialogs.md).
    QCoreApplication::setAttribute(Qt::AA_DontUseNativeDialogs);

    // On macOS, render offscreen unless the caller insisted otherwise. Cocoa
    // is not a supported way to produce this gallery — captures come out at
    // the display's devicePixelRatio and in the host's light/dark appearance,
    // so nothing compares to the tracked images — and the way that presents
    // is not "wrong pixels" but a run that wedges: a full-suite cocoa run
    // stalled indefinitely in `CaptureDialogs` once in two attempts
    // (2026-08-16), which is the event-dispatch livelock the CMake
    // regeneration target already forces offscreen to avoid. Defaulting it
    // here covers the invocation neither the ctest nets nor that target go
    // through — someone running the binary by hand, which has now cost two
    // sessions a diagnosis.
    //
    // Windows deliberately keeps its native platform: that is where the
    // published gallery is rendered, with real fonts. And an explicit
    // QT_QPA_PLATFORM still wins here, so previewing on screen stays possible
    // for anyone who asks for it on purpose.
#if defined(Q_OS_MACOS)
    if (qEnvironmentVariableIsEmpty("QT_QPA_PLATFORM"))
      qputenv("QT_QPA_PLATFORM", "offscreen");
#endif

    InitScreenshotOptions();
    FixtureConfig().Load(GetDataFilePath());
  }

  ScreenshotGenerator();
  ~ScreenshotGenerator();

 protected:
  // QApplication must exist before MessageLoopQt — the latter posts wakeup
  // events through it — so app_env_ is declared first.
  AppEnvironment app_env_;
  AnyExecutor executor_ = MakeAnyExecutor(std::make_shared<MessageLoopQt>());

  // Freeze scada::Now() at the fixture's `now` for the whole capture
  // run, so live-window rendering (table history windows, sparklines,
  // delivered-value timestamps) lines up with the fixture history — which is
  // laid out relative to that instant — and every timestamp in the output is
  // deterministic instead of the machine's wall clock. Declared before the
  // services/app members so nothing samples the real clock first. Qt/asio
  // timers use the steady clock and keep running normally.
  struct FixtureClock {
    FixtureClock() {
      const scada::Time now = FixtureNow(FixtureConfig().json);
      if (!scada::IsNull(now))
        override_.Advance(now - override_.Now());
    }
    scada::base::ScopedMockClockOverride override_;
  } fixture_clock_;

  // Russian translators, installed in the constructor body. Must outlive
  // the QApplication inside `app_env_`, hence declared right after it.
  // `qtbase_translator_` carries Qt's own strings (standard QMessageBox
  // buttons, spin/date chrome); it is installed before `translator_` so the
  // client catalog wins on conflicts.
  QTranslator qtbase_translator_;
  QTranslator translator_;

  // SCADA back-end. The address space starts pre-populated with the
  // standard OPC UA + SCADA folder/type tree (code-defined by
  // ScadaTestAddressSpace, no nodeset XML); ns=1 instance nodes from
  // `screenshot_data.json` get added on top in the test fixture's
  // constructor body.
  scada_test::ScadaTestAddressSpace address_space_;
  SyncAttributeServiceImpl sync_attribute_service_{
      AttributeServiceImplContext{address_space_}};
  AttributeServiceImpl attribute_service_{sync_attribute_service_};
  // The client passes an empty ServiceContext and a real server fills in the
  // session's identity. There is no server here, so an empty context reads as
  // anonymous and every permission-gated attribute is refused. Supply the
  // administrator identity the fixtures depict; see the header for why this
  // belongs to the fixture rather than to the client.
  //
  // Deliberately NOT installed into `services_`: changing the identity for
  // every read also changes UserAccessLevel narrowing, and with it any capture
  // that draws a writability affordance. It is handed only to the capture that
  // needs an administrator — the RBAC inspector, which cannot read the role ->
  // permission map without one.
  AuthenticatedAttributeService authenticated_attribute_service_{
      attribute_service_};
  SyncViewServiceImpl sync_view_service_{
      ViewServiceImplContext{address_space_}};
  ViewServiceImpl view_service_{sync_view_service_};
  scada::LocalHistoryService history_service_;
  // Delivers each subscribed node's actual address-space Value attribute
  // (e.g. TIT.212's base_value from screenshot_data.json) as the single
  // current-value sample, so dialog/graph current-value readouts render
  // meaningful, stable numbers.
  scada::LocalMonitoredItemService monitored_item_service_{
      sync_attribute_service_};
  scada::LocalMethodService method_service_;
  scada::LocalNodeManagementService node_management_service_;
  scada::LocalSessionService session_service_;

  scada::services services_{
      .attribute_service = &attribute_service_,
      .monitored_item_service = &monitored_item_service_,
      .method_service = &method_service_,
      .history_service = &history_service_,
      .view_service = &view_service_,
      .node_management_service = &node_management_service_,
      .session_service = &session_service_,
  };

  scada::base::ScopedPathOverride private_dir_override_{::client::DIR_PRIVATE};

  ClientApplication app_{ClientApplicationContext{
      .executor_ = executor_,
      .login_handler_ = [this](DataServicesContext&&)
          -> Awaitable<std::optional<DataServices>> {
        co_return std::optional{DataServices::FromUnownedServices(services_)};
      },
      // Intentionally no node_service/timed_data_service/tree-factory
      // overrides: we let ClientApplication build the production
      // `v1::NodeServiceImpl`, `TimedDataServiceImpl`, and default
      // `NodeServiceTreeImpl` on top of the Local* services. The node
      // graph is populated on demand by the address-space fetcher over
      // `LocalViewService::Browse` + `LocalAttributeService::Read`,
      // which is how current values, display names, and tree structure
      // all flow through the real code paths the client runs in prod.
      .module_configurator_ = MakeScreenshotModules()}};
};

}  // namespace scada::screenshot_generator
