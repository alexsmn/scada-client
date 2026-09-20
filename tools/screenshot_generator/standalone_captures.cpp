#include "standalone_captures.h"

#include "administration_capture.h"
#include "bulk_create_capture.h"
#include "command_field_capture.h"
#include "debugger_capture.h"
#include "device_diagnostics_capture.h"
#include "device_metrics_capture.h"
#include "frame_decode_capture.h"
#include "graph_capture.h"
#include "inspector_capture.h"
#include "severity_tiles_capture.h"
#include "shell_strip_capture.h"
#include "transmission_rule_capture.h"
#include "user_access_capture.h"

namespace scada::screenshot_generator {
namespace {

// The `capture` keys, in one place. This was seventeen hand-written `else if`
// arms until 2026-08-16; the shape cost a `++captured` once already (d8dd095c9
// inserted a branch between a comment and its `if`, so the run's tally
// under-reported by one from 2026-07-27 until it was found). A table cannot
// lose that line, because no entry carries it.
constexpr StandaloneCapture kStandaloneCaptures[] = {
    // The series inspector is standalone chrome, not a window on the page —
    // build it from the graph fixture instead of looking up an opened view.
    {"series-inspector",
     +[](const StandaloneCaptureContext& c) {
       SaveSeriesInspectorScreenshot(c.spec, c.executor, c.node_service,
                                     c.timed_data_service, c.json);
     }},
    // The device-diagnostics panel is standalone reshell chrome (the right
    // region of config-workbench.html), built from a fixture device rather
    // than an opened page view.
    {"device-diagnostics",
     +[](const StandaloneCaptureContext& c) {
       SaveDeviceDiagnosticsScreenshot(
           c.spec, c.node_service, c.timed_data_service, c.json, c.executor);
     }},
    // The device Metrics sheet is a CusTable whose cells DeviceMetricsModule
    // derives from the device's type-definition data variables, so it can only
    // be built once the node service has resolved the device — after the
    // profile page was assembled. It opens its own view.
    {"device-metrics",
     +[](const StandaloneCaptureContext& c) {
       SaveDeviceMetricsScreenshot(c.spec, c.main_window, c.node_service,
                                   c.timed_data_service, c.executor);
     }},
    // The Administration explorer is the left region of users-admin.html. It
    // derives its rows from the shell's command resolution, which the headless
    // generator has no shell for, so the capture supplies the section set.
    {"administration",
     +[](const StandaloneCaptureContext& c) {
       SaveAdministrationScreenshot(c.spec);
     }},
    // The users-admin RBAC inspector is standalone reshell chrome (the right
    // region of users-admin.html), built from a fixture user.
    {"user-access",
     +[](const StandaloneCaptureContext& c) {
       SaveUserAccessScreenshot(c.spec, c.node_service,
                                c.authenticated_attribute_service, c.executor);
     }},
    // The Roles view needs the same administrator identity: its grid is built
    // from the server's published role -> permission map, which an anonymous
    // session may not read. Opened as an ordinary view it rendered
    // "Roles · no data" and saved an empty grid.
    {"roles",
     +[](const StandaloneCaptureContext& c) {
       SaveRolesScreenshot(c.spec, c.node_service,
                           c.authenticated_attribute_service, c.executor);
     }},
    // The users-admin grid joins its Roles column from the same RoleSet read,
    // so it needs the same identity — through the view path every account's
    // Roles cell read "Нет данных". This is the standalone panel, not the
    // `users.png` Users window, which is a `type` spec and keeps going through
    // the profile page.
    {"users-grid",
     +[](const StandaloneCaptureContext& c) {
       SaveUsersGridScreenshot(c.spec, c.node_service,
                               c.authenticated_attribute_service, c.executor);
     }},
    // The transmission-rule inspector is standalone reshell chrome (the right
    // region of transmission-rules.html), built from a fixture transmission
    // item.
    {"transmission-rule",
     +[](const StandaloneCaptureContext& c) {
       SaveTransmissionRuleScreenshot(c.spec, c.node_service, c.executor);
     }},
    // The bulk-create preview is standalone reshell chrome (the center of
    // bulk-create.html), built from a demo pattern with no node service.
    {"bulk-create",
     +[](const StandaloneCaptureContext& c) {
       SaveBulkCreateScreenshot(c.spec);
     }},
    // The protocol debugger is a --debug-gated window, not a registered view,
    // so the ordinary view sweep cannot reach it; it is built over a fixture
    // request trace.
    {"debugger",
     +[](const StandaloneCaptureContext& c) {
       SaveDebuggerScreenshot(c.spec);
     }},
    // The device-log filter bar is a strip of stock widgets, built on its own
    // rather than reached through WatchView.
    {"watch-filter-bar",
     +[](const StandaloneCaptureContext& c) {
       SaveWatchFilterBarScreenshot(c.spec);
     }},
    // The frame-decode pane is the device log's inspector; it is built over a
    // fixture APDU because reaching it through WatchView would mean assembling
    // a full ControllerContext.
    {"frame-decode",
     +[](const StandaloneCaptureContext& c) {
       SaveFrameDecodeScreenshot(c.spec);
     }},
    // The KPI severity tiles are standalone reshell chrome (the context bar's
    // alarm summary), built from seeded counts with no node service.
    {"severity-tiles",
     +[](const StandaloneCaptureContext& c) {
       SaveSeverityTilesScreenshot(c.spec);
     }},
    // The command/search field is standalone reshell chrome (the context bar's
    // palette entry point), built with the same prompt and shortcut the window
    // gives it.
    {"command-field",
     +[](const StandaloneCaptureContext& c) {
       SaveCommandFieldScreenshot(c.spec);
     }},
    // The three shell strips: each is a thin band inside a 1920px window, so
    // each gets a picture of its own rather than being read out of
    // workbench-window.png. The two rail strips build an ActivityBar with only
    // their own zone populated.
    {"pages",
     +[](const StandaloneCaptureContext& c) { SavePagesScreenshot(c.spec); }},
    {"rail-utilities",
     +[](const StandaloneCaptureContext& c) {
       SaveRailUtilitiesScreenshot(c.spec);
     }},
    {"breadcrumb",
     +[](const StandaloneCaptureContext& c) {
       SaveBreadcrumbScreenshot(c.spec);
     }},
    // The Inspector is standalone reshell chrome (the right-hand selection
    // panel), filled with a representative expression-row selection.
    {"inspector",
     +[](const StandaloneCaptureContext& c) {
       SaveInspectorScreenshot(c.spec);
     }},
    // The Inspector's event (alarm) card for a journal-row selection.
    {"inspector-event",
     +[](const StandaloneCaptureContext& c) {
       SaveInspectorEventScreenshot(c.spec);
     }},
    // The substation display needs a DisplayFrame of its own and is rendered
    // by the CaptureDisplay TEST_F; nothing for this sweep to grab. Null
    // rather than absent, so an unknown key is still an error.
    {"display", nullptr},
    // Likewise the settings overlay: it is shown over a real main window by
    // the CaptureSettingsPanel TEST_F, which this sweep has no window for.
    {"settings", nullptr},
};

}  // namespace

const StandaloneCapture* FindStandaloneCapture(std::string_view key) {
  for (const StandaloneCapture& entry : kStandaloneCaptures) {
    if (entry.key == key)
      return &entry;
  }
  return nullptr;
}

}  // namespace scada::screenshot_generator
