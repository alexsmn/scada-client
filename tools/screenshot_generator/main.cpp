#include "administration_capture.h"
#include "authenticated_attribute_service.h"
#include "bulk_create_capture.h"
#include "command_field_capture.h"
#include "debugger_capture.h"
#include "device_diagnostics_capture.h"
#include "device_metrics_capture.h"
#include "dialog_capture.h"
#include "display_capture.h"
#include "fixture_builder.h"
#include "frame_decode_capture.h"
#include "graph_capture.h"
#include "inspector_capture.h"
#include "null_task_manager.h"
#include "screenshot_config.h"
#include "screenshot_modules.h"
#include "screenshot_options.h"
#include "screenshot_output.h"
#include "screenshot_wait.h"
#include "severity_tiles_capture.h"
#include "transmission_rule_capture.h"
#include "user_access_capture.h"
#include "widget_capture.h"

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
#include "aui/qt/theme_qt.h"
#include "aui/severity_colors.h"
#include "aui/test/app_environment.h"
#include "aui/translation.h"
#include "aui/tree.h"
#include "base/any_executor.h"
#include "base/client_paths.h"
#include "base/no_destructor.h"
#include "base/test/scoped_mock_clock_override.h"
#include "base/test/scoped_path_override.h"
#include "base/utf_convert.h"
#include "controller/window_info.h"
#include "events/qt/event_filter_bar.h"
#include "favorites/favourites.h"
#include "main_window/main_menu/main_menu_model.h"
#include "main_window/main_window.h"
#include "main_window/main_window_manager.h"
#include "main_window/opened_view/opened_view.h"
#include "main_window/settings_dialog_qt.h"
#include "model/data_items_node_ids.h"
#include "model/devices_node_ids.h"
#include "model/node_id_util.h"
#include "model/security_node_ids.h"
#include "modules/limits/limit_model.h"
#include "modules/transmission/transmission_devices.h"
#include "modules/write/write_model.h"
#include "node_service/node_awaitable.h"
#include "node_service/node_ref.h"
#include "node_service/node_service.h"
#include "node_service/node_util.h"
#include "profile/profile.h"
#include "profile/window_definition.h"
#include "timed_data/timed_data_service.h"
#include "user_access/role_membership.h"

#include <QAbstractButton>
#include <QAbstractProxyModel>
#include <QAction>
#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDockWidget>
#include <QElapsedTimer>
#include <QHeaderView>
#include <QLayout>
#include <QLibraryInfo>
#include <QLocale>
#include <QMainWindow>
#include <QMenu>
#include <QMenuBar>
#include <QPixmap>
#include <QSettings>
#include <QStandardItem>
#include <QStandardItemModel>
#include <QString>
#include <QTableView>
#include <QTemporaryDir>
#include <QToolBar>
#include <QToolButton>
#include <QTranslator>
#include <QTreeView>
#include <QVBoxLayout>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <algorithm>
#include <set>

namespace {

using scada::screenshot_generator::WaitForAwaitable;
using scada::screenshot_generator::WaitForPendingNodeLoads;

// Global config loaded once per test suite.
ScreenshotConfig g_config;

template <class Predicate>
bool WaitUntil(Predicate&& predicate, int timeout_ms = 5000) {
  QElapsedTimer timer;
  timer.start();
  while (!predicate()) {
    if (timer.elapsed() >= timeout_ms)
      return false;
    QApplication::processEvents(QEventLoop::AllEvents, 50);
  }
  return true;
}

// Pulls the data items a spec names (`path` plus `paths`) fully resident, so a
// capture does not depend on another window in the same run having browsed
// them first. Non-node paths (formulas, unresolvable ids) simply yield no node
// id and are skipped.
void MakeSpecItemsResident(NodeService& node_service,
                           const ScreenshotSpec& spec) {
  std::vector<scada::NodeId> node_ids;
  auto add = [&node_ids](const std::string& path) {
    if (path.empty())
      return;
    scada::NodeId node_id = NodeIdFromScadaString(path);
    if (!node_id.is_null())
      node_ids.push_back(std::move(node_id));
  };

  add(spec.path);
  for (const auto& path : spec.paths)
    add(path);
  // A spreadsheet names its items nowhere else: its live cells are
  // "=<formula>" text. Without warming them the sheet renders a raw value
  // where a fuller run — one where another window happened to browse the same
  // node — renders the item's display format.
  for (const auto& cell : spec.cells) {
    if (cell.text.starts_with('='))
      add(cell.text.substr(1));
  }

  if (node_ids.empty())
    return;

  scada::screenshot_generator::FetchNodesResident(node_service, node_ids);
}

// Seeds the device-log lines the fixture's `device_log` block defines as event
// notifications on the device, so the Log capture renders a real session
// instead of an empty grid. A `frame` object makes the line arrive as a
// DeviceFrameEvent, which is what fills the decoded columns; without one it is
// a plain log line, the shape an older server sends.
void SeedDeviceLog(const boost::json::value& root,
                   scada::LocalMonitoredItemService& monitored_item_service) {
  const auto* block = root.as_object().if_contains("device_log");
  if (!block)
    return;

  const scada::NodeId device_id =
      NodeIdFromScadaString(std::string_view(block->at("device").as_string()));
  const scada::Time now = scada::Now();

  for (const auto& jl : block->at("lines").as_array()) {
    scada::Event event;
    event.event_id = 1;
    const double seconds_ago = jl.at("seconds_ago").to_number<double>();
    event.time = now - std::chrono::round<std::chrono::microseconds>(
                           std::chrono::duration<double>{seconds_ago});
    event.receive_time = event.time;
    event.source_node_id = device_id;
    event.event_type_id = scada::devices::id::DeviceWatchEventType;
    event.message = scada::LocalizedText{
        UtfConvert<char16_t>(std::string(jl.at("message").as_string()))};
    event.severity = scada::kSeverityMin;
    if (const auto* js = jl.as_object().if_contains("severity")) {
      const std::string_view severity = js->as_string();
      if (severity == "warning")
        event.severity = scada::kSeverityWarning;
      else if (severity == "error")
        event.severity = scada::kSeverityCritical;
    }

    const auto* jf = jl.as_object().if_contains("frame");
    if (!jf) {
      monitored_item_service.AddEvent(device_id, std::any{std::move(event)});
      continue;
    }

    event.event_type_id = scada::devices::id::DeviceFrameEventType;

    scada::DeviceFrame frame;
    const std::string_view direction = jf->at("direction").as_string();
    frame.direction = direction == "tx" ? scada::DeviceFrame::kOutbound
                                        : scada::DeviceFrame::kInbound;
    const auto read = [jf](std::string_view name) -> scada::Int32 {
      const auto* value = jf->as_object().if_contains(name);
      return value ? static_cast<scada::Int32>(value->to_number<std::int64_t>())
                   : 0;
    };
    if (const auto* jfmt = jf->as_object().if_contains("format"))
      frame.format = std::string(jfmt->as_string());
    frame.type_id = read("type_id");
    frame.cause = read("cause");
    frame.object_address = read("object_address");
    frame.send_sequence = read("send_sequence");
    frame.receive_sequence = read("receive_sequence");

    monitored_item_service.AddEvent(
        device_id, std::any{scada::DeviceFrameEvent{
                       .base = std::move(event), .frame = std::move(frame)}});
  }
}

// Seeds the Favorites pane from the fixture's `favourites` block. The pane
// renders whatever the profile holds, and the generator starts from an empty
// one — so without this the capture is a blank panel, which is how it shipped
// for months.
void SeedFavourites(const boost::json::value& root, Favourites& favourites) {
  const auto* block = root.as_object().if_contains("favourites");
  if (!block)
    return;

  for (const auto& jf : block->at("folders").as_array()) {
    const std::u16string name =
        UtfConvert<char16_t>(std::string(jf.at("name").as_string()));
    const Page& folder = favourites.GetOrAddFolder(name);

    for (const auto& jw : jf.at("windows").as_array()) {
      WindowDefinition window{std::string(jw.at("type").as_string())};
      window.title =
          UtfConvert<char16_t>(std::string(jw.at("title").as_string()));
      favourites.Add(window, folder);
    }
  }
}

scada::aui::Tree* FindTreeWidget(QWidget* widget) {
  if (!widget)
    return nullptr;

  if (auto* tree = dynamic_cast<scada::aui::Tree*>(widget))
    return tree;

  for (auto* child : widget->findChildren<QWidget*>()) {
    if (auto* tree = dynamic_cast<scada::aui::Tree*>(child))
      return tree;
  }

  return nullptr;
}

// The grid widget a view renders into, itself or the first one below it.
QTableView* FindGridWidget(QWidget* widget) {
  if (!widget)
    return nullptr;
  if (auto* table = qobject_cast<QTableView*>(widget))
    return table;
  return widget->findChild<QTableView*>();
}

// Cells of a grid model whose display text is not empty.
int CountFilledCells(const QAbstractItemModel& model) {
  int filled = 0;
  for (int row = 0; row < model.rowCount(); ++row) {
    for (int column = 0; column < model.columnCount(); ++column) {
      if (!model.index(row, column).data(Qt::DisplayRole).toString().isEmpty())
        ++filled;
    }
  }
  return filled;
}

// Rows a tree model has materialized under `parent`, descendants included.
// Reads `rowCount` only — a lazy tree fetches through `canFetchMore`/
// `fetchMore`, so counting never pulls in rows the capture would not show.
int CountLoadedRows(const QAbstractItemModel& model,
                    const QModelIndex& parent) {
  const int count = model.rowCount(parent);
  int total = count;
  for (int row = 0; row < count; ++row)
    total += CountLoadedRows(model, model.index(row, 0, parent));
  return total;
}

}  // namespace

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

    InitScreenshotOptions();
    g_config.Load(GetDataFilePath());
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
      const scada::Time now = FixtureNow(g_config.json);
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

  scada::base::ScopedPathOverride private_dir_override_{client::DIR_PRIVATE};

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

ScreenshotGenerator::ScreenshotGenerator() {
  // Install the Russian translator directly. We intentionally bypass
  // InstalledTranslation here: it reads the locale from a QSettings
  // instance that lacks an organization/app name (AppEnvironment builds
  // a bare QApplication), so the value round-trips through an
  // unreliable backing store and the Russian .qm ends up not loading
  // at all.
  //
  // The install must happen per-fixture: each TEST_F spins up a fresh
  // QApplication via AppEnvironment, and QTranslator registrations
  // don't survive across QApplication instances.
  QLocale::setDefault(QLocale{QLocale::Russian, QLocale::Russia});

  // Record that choice where the client reads it back. `GetSelectedLocaleName`
  // (main_window/main_window_module.cpp) falls back to `QLocale::system()` when
  // the setting is absent, and the hermetic settings tree above is empty on
  // every run — so the Language row rendered whatever locale the CAPTURING
  // MACHINE happened to use while the labels beside it were always Russian.
  // That is how settings-dialog.png shipped reading "Язык: English". Pinning it
  // makes the rendered locale a property of the fixture rather than of the
  // host, which is the same reason the style below is pinned to Fusion.
  QSettings{}.setValue("LocaleName", "ru_RU");

  const auto translation_dir =
      QApplication::applicationDirPath() + "/translations";
  // Qt's own catalog first (standard Yes/No/Cancel buttons, spin/date
  // chrome), mirroring InstalledTranslation: prefer Qt's installed
  // translations dir, fall back to the staged app dir. Installed before the
  // client catalog so the client strings win on conflicts.
  if (qtbase_translator_.load(
          "qtbase_ru", QLibraryInfo::path(QLibraryInfo::TranslationsPath)) ||
      qtbase_translator_.load("qtbase_ru", translation_dir)) {
    QApplication::installTranslator(&qtbase_translator_);
  }
  if (translator_.load("client_ru", translation_dir))
    QApplication::installTranslator(&translator_);

  // Pin Fusion for captures. Unlike the client — which runs the platform style
  // so it looks native (docs/client/ux/principles.md §9) — published
  // screenshots must be byte-comparable across machines, so they deliberately
  // do NOT follow the host style. Set directly rather than through
  // InstalledStyle for the QSettings reason: that also writes back the live
  // style's objectName on destruction, which would let the second TEST_F pick
  // up whatever QStyleFactory returned instead of "Fusion".
  //
  // Consequence to keep in mind while the native migration runs
  // (docs/client/ux/backlog.md P6): these captures show the Fusion rendering,
  // not what an operator sees. Validate native-look changes by running the
  // real client, not only by diffing generated PNGs.
  QApplication::setStyle("Fusion");

  // Optionally render under a UX design-token theme so captures validate the
  // reshell against real Qt widgets (--theme=dark|light|hc). Applied over the
  // Fusion base exactly as the client does when the experimental UX is on (see
  // app/qt/installed_appearance.h). ApplyTheme settles the severity/quality
  // ramp to match; this used to repeat that mapping by hand.
  if (const std::string& theme_name = GetScreenshotOptions().theme;
      !theme_name.empty()) {
    // Resolve `system` here: a capture must pin one concrete appearance,
    // never follow the machine that happens to render it. Resolving also stops
    // ApplyTheme installing the system-following watcher, which would let the
    // host desktop change a capture mid-run.
    scada::aui::ApplyTheme(scada::aui::ResolveTheme(scada::aui::ThemeFromString(
        QString::fromStdString(theme_name), scada::aui::Theme::kDark)));
  }

  // Render offscreen. `widget->grab()` renders the Qt widget tree to a
  // QPixmap without needing the window to be on-screen — so this
  // suite works headless (incl. CI without a desktop session).
  // Trade-off: we don't get the native OS title bar / frame; we tried
  // PrintWindow(PW_RENDERFULLCONTENT) for that and it didn't reliably
  // capture child-widget content (see commit history).
  MainWindow::SetHideForTesting();

  // Populate the address space with ns=1 instance nodes; everything
  // else (standard OPC UA / SCADA tree) was already built in code by
  // ScadaTestAddressSpace. The real `v1::NodeServiceImpl`
  // inside ClientApplication browses and reads them through
  // ViewServiceImpl + AttributeServiceImpl on demand.
  PopulateFixtureNodes(address_space_, g_config.json);
  // The Users grid reads the standard model; project the fixture's accounts
  // onto it so the capture is not an empty grid.
  ProjectFixtureUsersOntoStandardModel(address_space_, g_config.json);

  // Seed history only for nodes that made it into the address space. The
  // fixture's `nodes` array can declare a node its `tree` never parents, and
  // PopulateFixtureNodes cannot create such an orphan — without this gate the
  // history service would still synthesize a series for it, and a table row
  // bound to that node would paint a convincing sparkline next to a "no data"
  // quality mark.
  history_service_.LoadFromJson(
      g_config.json, [this](const scada::NodeId& node_id) {
        return address_space_.GetNode(node_id) != nullptr;
      });

  SeedDeviceLog(g_config.json, monitored_item_service_);
}

ScreenshotGenerator::~ScreenshotGenerator() {
  WaitForAwaitable(executor_, app_.Quit());
}

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
    profile.AddPage(MakeScreenshotPage(g_config.screenshots, g_config.json));
    profile.Save();
  }

  WaitForAwaitable(executor_, app_.Start());

  // After Start, because the favourites store is built during post-login. The
  // pane's model subscribes to additions, so rows added now still reach it.
  SeedFavourites(g_config.json, app_.favourites());

  // Wait for the data itself rather than pumping for a fixed second and
  // hoping: with many windows open that second was split too many ways, and a
  // view could be grabbed before its trends arrived.
  ASSERT_TRUE(scada::screenshot_generator::WaitForPendingData(
      app_.node_service(), app_.timed_data_service()));

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
  for (const auto& spec : g_config.screenshots) {
    // The series inspector is standalone chrome, not a window on the page —
    // build it from the graph fixture instead of looking up an opened view.
    if (spec.window_type == "SeriesInspector") {
      SaveSeriesInspectorScreenshot(spec, app_.node_service(),
                                    app_.timed_data_service(), g_config.json);
      ++captured;
      continue;
    }
    // The device-diagnostics panel is standalone reshell chrome (the right
    // region of config-workbench.html), built from a fixture device rather than
    // an opened page view.
    if (spec.window_type == "DeviceDiagnostics") {
      SaveDeviceDiagnosticsScreenshot(spec, app_.node_service(),
                                      app_.timed_data_service(), g_config.json);
      ++captured;
      continue;
    }
    // The device Metrics sheet is a CusTable whose cells DeviceMetricsModule
    // derives from the device's type-definition data variables, so it can only
    // be built once the node service has resolved the device — after the
    // profile page was assembled. It opens its own view here.
    if (spec.window_type == "DeviceMetrics") {
      SaveDeviceMetricsScreenshot(spec, main_window, app_.node_service(),
                                  app_.timed_data_service(), executor_);
      ++captured;
      continue;
    }
    // The Administration explorer is the left region of users-admin.html. It
    // derives its rows from the shell's command resolution, which the headless
    // generator has no shell for, so the capture supplies the section set.
    if (spec.window_type == "Administration") {
      SaveAdministrationScreenshot(spec);
      ++captured;
      continue;
    }
    // The users-admin RBAC inspector is standalone reshell chrome (the right
    // region of users-admin.html), built from a fixture user.
    if (spec.window_type == "UserAccess") {
      SaveUserAccessScreenshot(spec, app_.node_service(),
                               authenticated_attribute_service_, executor_);
      ++captured;
      continue;
    }
    // The Roles view needs the same administrator identity: its grid is built
    // from the server's published role -> permission map, which an anonymous
    // session may not read. Opened as an ordinary view it rendered "Roles · no
    // data" and saved an empty grid.
    if (spec.window_type == "Roles") {
      SaveRolesScreenshot(spec, app_.node_service(),
                          authenticated_attribute_service_, executor_);
      ++captured;
      continue;
    }
    // The users-admin grid joins its Roles column from the same RoleSet read,
    // so it needs the same identity — through the view path every account's
    // Roles cell read "Нет данных". Only under `--theme`: the legacy
    // `users.png` is the classic administration view, not this panel (which
    // `MakeUsersGridPanel` does not build outside the reshell at all), and it
    // must keep going through the profile page.
    if (spec.window_type == "Users" && !GetScreenshotOptions().theme.empty()) {
      SaveUsersGridScreenshot(spec, app_.node_service(),
                              authenticated_attribute_service_, executor_);
      ++captured;
      continue;
    }
    // The transmission-rule inspector is standalone reshell chrome (the right
    // region of transmission-rules.html), built from a fixture transmission
    // item.
    if (spec.window_type == "TransmissionRule") {
      SaveTransmissionRuleScreenshot(spec, app_.node_service());
      ++captured;
      continue;
    }
    // The bulk-create preview is standalone reshell chrome (the center of
    // bulk-create.html), built from a demo pattern with no node service.
    if (spec.window_type == "BulkCreate") {
      SaveBulkCreateScreenshot(spec);
      ++captured;
      continue;
    }
    // The protocol debugger is a --debug-gated window, not a registered view,
    // so the ordinary view sweep cannot reach it; it is built here over a
    // fixture request trace.
    if (spec.window_type == "Debugger") {
      SaveDebuggerScreenshot(spec);
      ++captured;
      continue;
    }
    // The device-log filter bar is a strip of stock widgets, built here on its
    // own rather than reached through WatchView.
    if (spec.window_type == "WatchFilterBar") {
      SaveWatchFilterBarScreenshot(spec);
      ++captured;
      continue;
    }
    // The frame-decode pane is the device log's inspector; it is built here
    // over a fixture APDU because reaching it through WatchView would mean
    // assembling a full ControllerContext.
    if (spec.window_type == "FrameDecode") {
      SaveFrameDecodeScreenshot(spec);
      ++captured;
      continue;
    }
    // The KPI severity tiles are standalone reshell chrome (the context bar's
    // alarm summary), built from seeded counts with no node service.
    if (spec.window_type == "SeverityTiles") {
      SaveSeverityTilesScreenshot(spec);
      ++captured;
      continue;
    }
    // The command/search field is standalone reshell chrome (the context bar's
    // palette entry point), built with the same prompt and shortcut the window
    // gives it.
    if (spec.window_type == "CommandField") {
      SaveCommandFieldScreenshot(spec);
      ++captured;
      continue;
    }
    // The Inspector is standalone reshell chrome (the right-hand selection
    // panel), filled with a representative expression-row selection.
    if (spec.window_type == "Inspector") {
      SaveInspectorScreenshot(spec);
      ++captured;
      continue;
    }
    // The Inspector's event (alarm) card for a journal-row selection.
    if (spec.window_type == "InspectorEvent") {
      SaveInspectorEventScreenshot(spec);
      ++captured;
      continue;
    }
    // The substation display is rendered standalone (CaptureDisplay) — it is
    // not opened as a page view, so skip it in the view-matching loop.
    if (spec.window_type == "Display")
      continue;

    // A sidebar pane is only on screen while its activity-rail mode is
    // selected — the rail is authoritative over the left dock (see
    // main_window/pane_modes.h). Select the owning mode first, exactly as an
    // operator would, so the pane exists to be captured.
    if (main_window.SelectPaneModeForPane(spec.window_type)) {
      // The switch destroyed the previous mode's panes. `used_views` keys on
      // raw pointers, and a freshly created view can land on a freed address,
      // so a stale entry would make the new pane look already-consumed. The
      // set only disambiguates specs that share a window_type within one
      // layout, so dropping it at a mode boundary loses nothing.
      used_views.clear();
    }

    OpenedView* view = nullptr;
    for (OpenedView* v : main_window.opened_views()) {
      if (v->window_info().name == spec.window_type &&
          !used_views.contains(v)) {
        view = v;
        break;
      }
    }

    if (!view) {
      ADD_FAILURE() << "Window type not found: " << spec.window_type;
      continue;
    }
    used_views.insert(view);

    QWidget* widget = view->view();
    if (!widget) {
      ADD_FAILURE() << "No QWidget for: " << spec.window_type;
      continue;
    }

    // Make this spec's own items fully resident before grabbing. A row's
    // display format, engineering units and limit bands are property children
    // fetched on demand, and TimedData only pulls the item node itself
    // (NodeOnly) — so a row renders unformatted until something else browses
    // those children. In a full run the Explorer tree does that incidentally,
    // which made a capture's correctness depend on which *other* windows the
    // run happened to include: `--only table.png` rendered Частота as "50"
    // where a wider run rendered the fixture's "0.00" format as "50.01".
    // Warming the spec's own nodes makes each capture self-sufficient.
    MakeSpecItemsResident(app_.node_service(), spec);

    // Those fetches can in turn start history reads (a row's alias resolves,
    // then asks for its sparkline window), so settle again before grabbing.
    EXPECT_TRUE(scada::screenshot_generator::WaitForPendingData(
        app_.node_service(), app_.timed_data_service()))
        << spec.filename;

    // A collapsed tree captures its folders and hides everything the capture
    // is about, so a tree-backed spec can ask for every row. Done before the
    // row check below, which must count what the capture will actually show:
    // a lazy tree has only materialized its root until something expands it.
    if (spec.expand) {
      if (scada::aui::Tree* tree = FindTreeWidget(widget)) {
        tree->expandAll();
        QApplication::processEvents();
        // expandAll fetches the next level lazily, so each newly shown row can
        // start its own child browse; settle those before counting or grabbing.
        EXPECT_TRUE(WaitForPendingNodeLoads(app_.node_service()))
            << spec.filename;
        tree->expandAll();
        QApplication::processEvents();
      } else {
        ADD_FAILURE() << spec.filename << ": expand was requested but "
                      << spec.window_type << " has no tree";
      }
    }

    // A grid-backed window that renders fewer rows than the fixture defines
    // is a data-path regression (empty users/transmission tables have
    // shipped as "successful" captures before) — fail loudly instead of
    // silently saving a bare frame.
    if (spec.min_rows > 0 || spec.exact_rows > 0 || spec.min_columns > 0) {
      int max_rows = 0;
      int max_columns = 0;
      const auto count_grid = [&] {
        max_rows = 0;
        max_columns = 0;
        QList<QTableView*> tables = widget->findChildren<QTableView*>();
        if (auto* table = qobject_cast<QTableView*>(widget))
          tables.prepend(table);
        for (const QTableView* table : tables) {
          if (table->model()) {
            max_rows = std::max(max_rows, table->model()->rowCount());
            max_columns = std::max(max_columns, table->model()->columnCount());
          }
        }
        // Tree-backed windows count every row they have materialized, not just
        // the top level. Without this a tree capture can go empty as silently
        // as a grid one — which is exactly how the favourites pane shipped
        // blank — and a top-level-only count says nothing about a tree whose
        // single root row is the shell and whose content is its children (the
        // Files view, where an empty file store still shows one row).
        if (const scada::aui::Tree* tree = FindTreeWidget(widget)) {
          if (tree->model())
            max_rows = std::max(
                max_rows, CountLoadedRows(*tree->model(), tree->rootIndex()));
        }
      };

      // A panel that fills itself from its own async read is not populated at
      // this point, and nothing above waits for it: WaitForPendingData only
      // sees work the node service has already been asked for, and a view that
      // CoSpawns its read at construction has not necessarily issued it yet
      // when we get here. The Roles panel does exactly that
      // (client/modules/user_access/qt/roles_view.cpp) and counted 0 rows
      // against a fixture that defines two — failing the check AND saving an
      // empty capture, since the grab below happens after this.
      //
      // So settle before judging: pump until the spec's own expectation is
      // met, then assert. A genuinely empty grid still fails, one second
      // later. This cannot paper over a row *leak* (the exact_rows case), for
      // which the wait stops at the first satisfying count and the assertion
      // below is unchanged.
      // PumpEventLoopFor, not the processEvents-based WaitUntil above: the
      // population lands as a MessageLoopQt-scheduled continuation, and on
      // macOS the processEvents forms do not fire timers on a drained queue
      // (see screenshot_wait.h).
      constexpr int kGridSettleTimeoutMs = 10'000;
      QElapsedTimer settle;
      settle.start();
      for (;;) {
        count_grid();
        const bool satisfied =
            (spec.min_rows == 0 || max_rows >= spec.min_rows) &&
            (spec.exact_rows == 0 || max_rows == spec.exact_rows) &&
            (spec.min_columns == 0 || max_columns >= spec.min_columns);
        if (satisfied || settle.elapsed() >= kGridSettleTimeoutMs)
          break;
        scada::screenshot_generator::PumpEventLoopFor(
            std::chrono::milliseconds{50});
      }

      if (spec.min_rows > 0) {
        EXPECT_GE(max_rows, spec.min_rows)
            << spec.filename << ": the " << spec.window_type
            << " grid rendered fewer rows than the fixture populates - the "
               "capture would be empty or partial";
      }
      // The exact expectation additionally catches rows leaking IN from
      // outside the captured scope (the transmission grid once picked up
      // another device's rules from a global model-change event).
      if (spec.exact_rows > 0) {
        EXPECT_EQ(max_rows, spec.exact_rows)
            << spec.filename << ": the " << spec.window_type
            << " grid row count does not match the fixture";
      }
      // A column-per-item grid (the summary) goes empty without losing a
      // single row, so the row checks above cannot see it.
      if (spec.min_columns > 0) {
        EXPECT_GE(max_columns, spec.min_columns)
            << spec.filename << ": the " << spec.window_type
            << " grid rendered fewer columns than the fixture configures - the "
               "capture would show its axis and no data";
      }
    }

    // A spreadsheet is a fixed grid of mostly-empty cells, so neither its row
    // nor its column count moves when its content goes missing — an empty
    // sheet and a full one are the same shape. Count the cells that actually
    // rendered text instead, against the ones the fixture writes. A cell bound
    // to a live value ("=<formula>") that resolves to nothing counts as
    // missing, which is the point.
    if (!spec.cells.empty()) {
      const auto expected = std::ranges::count_if(
          spec.cells,
          [](const SheetCellSpec& cell) { return !cell.text.empty(); });
      int filled = 0;
      if (auto* table = FindGridWidget(widget)) {
        if (const QAbstractItemModel* model = table->model())
          filled = CountFilledCells(*model);
      }
      EXPECT_GE(filled, expected)
          << spec.filename << ": the " << spec.window_type << " sheet rendered "
          << filled << " non-empty cells, fewer than the " << expected
          << " the fixture writes - the capture would be a blank "
             "grid";
    }

    // Optionally click a named child (e.g. a subtab button) so the capture
    // shows a non-default tab of a multi-tab view. Done before SaveScreenshot
    // detaches the widget for the grab.
    if (!spec.click_object.empty()) {
      // The subtab buttons are built when the reshell form finishes its async
      // node browse, which may not have completed yet for a just-opened view;
      // wait for the named button before clicking.
      QAbstractButton* button = nullptr;
      WaitUntil([&] {
        button = widget->findChild<QAbstractButton*>(
            QString::fromStdString(spec.click_object));
        return button != nullptr;
      });
      if (button) {
        button->click();
        QApplication::processEvents();
      } else {
        ADD_FAILURE() << "click_object not found: " << spec.click_object
                      << " in " << spec.window_type;
      }
    }

    if (spec.window_type == "Graph") {
      // Render graph standalone — hidden main windows don't lay out
      // QSplitter children, so we build a fresh graph widget.
      SaveGraphScreenshot(spec, app_.node_service(), app_.timed_data_service(),
                          g_config.json);
    } else {
      SaveScreenshot(widget, spec);
    }
    ++captured;
  }

  std::cout << "Captured " << captured << "/" << g_config.screenshots.size()
            << " screenshots to " << output_dir.string() << std::endl;
}

TEST_F(ScreenshotGenerator, CaptureDisplay) {
  // The reshelled substation display renders standalone from a VDS fixture — it
  // isn't part of the profile page, so it can't be picked up by
  // CaptureAllWindows' view-matching loop. Cross-platform: the VDS renderer
  // paints without the Windows-only Modus/Vidicon ActiveX host.
  const ScreenshotSpec* display_spec = nullptr;
  for (const auto& spec : g_config.screenshots) {
    if (spec.window_type == "Display") {
      display_spec = &spec;
      break;
    }
  }
  if (!display_spec || !ShouldCaptureScreenshot(display_spec->filename))
    GTEST_SKIP() << "Display capture not requested";

  // The bay strips need the live services, so the app runs for this capture
  // exactly as it does for the view captures.
  WaitForAwaitable(executor_, app_.Start());
  ASSERT_TRUE(WaitForPendingNodeLoads(app_.node_service()));

  std::filesystem::create_directories(GetOutputDir());
  SaveDisplayScreenshot(*display_spec, g_config.json, app_.timed_data_service(),
                        app_.node_event_provider(), app_.node_service());
}

TEST_F(ScreenshotGenerator, CaptureMainWindow) {
  // Under --theme the same capture renders the reshelled operator workbench
  // (activity rail, context bar with the severity tiles, editor tabs, status
  // strip) and is published as its own image, so the legacy client-window.png
  // (hand-maintained for the manual until the fake Modus runtime exists) is
  // never overwritten by a themed render.
  const char* filename = GetScreenshotOptions().theme.empty()
                             ? "client-window.png"
                             : "workbench-window.png";
  if (!ShouldCaptureScreenshot(filename))
    GTEST_SKIP() << filename << " not requested";

  MainWindow::SetHideForTesting(false);

  auto output_dir = GetOutputDir();
  std::filesystem::create_directories(output_dir);
  const auto output_image = output_dir / filename;

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
  ASSERT_TRUE(WaitForPendingNodeLoads(app_.node_service()));

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
  // Target the command toolbar specifically: the opt-in reshell adds other
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
  for (OpenedView* view : main_window.opened_views()) {
    if (view->window_info().name != "Struct")
      continue;

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
  tree->show();

  for (int i = 0; i < 10; ++i)
    QApplication::processEvents();

  QModelIndex root_index = tree->model()->index(0, 0, tree->rootIndex());
  if (!root_index.isValid())
    root_index = tree->model()->index(0, 0);
  ASSERT_TRUE(root_index.isValid());

  if (tree->model()->canFetchMore(root_index))
    tree->model()->fetchMore(root_index);
  ASSERT_TRUE(WaitForPendingNodeLoads(app_.node_service()));

  // For the screenshot we want the object rows, not the synthetic tree root.
  // Making the fetched root the view root sidesteps the "expanded root with
  // empty child viewport" state that Qt sometimes gets into here.
  tree->setRootIndex(root_index);
  tree->setRootIsDecorated(true);
  tree->expand(tree->rootIndex());
  ASSERT_TRUE(WaitUntil([&] { return tree->isExpanded(tree->rootIndex()); }));
  tree->expandRecursively(tree->rootIndex(), 3);
  tree->resizeColumnToContents(0);
  tree->doItemsLayout();
  tree->viewport()->update();

  auto* proxy_model = qobject_cast<QAbstractProxyModel*>(tree->model());
  QModelIndex materialized_source_root;
  const bool first_child_visible = WaitUntil(
      [&] {
        const auto& visible_root = tree->rootIndex();
        if (tree->model()->canFetchMore(visible_root))
          tree->model()->fetchMore(visible_root);

        auto first_child = tree->model()->index(0, 0, visible_root);
        if (!first_child.isValid() && proxy_model) {
          materialized_source_root = proxy_model->mapToSource(visible_root);
          if (materialized_source_root.isValid()) {
            const auto source_first_child = proxy_model->sourceModel()->index(
                0, 0, materialized_source_root);
            if (source_first_child.isValid())
              first_child = proxy_model->mapFromSource(source_first_child);
          }

          if (!first_child.isValid() && materialized_source_root.isValid() &&
              proxy_model->sourceModel()->rowCount(materialized_source_root) >
                  0) {
            tree->model()->sort(0);
            tree->collapse(visible_root);
            QApplication::processEvents();
            tree->expand(visible_root);
            first_child = tree->model()->index(0, 0, visible_root);

            if (!first_child.isValid()) {
              const auto source_first_child = proxy_model->sourceModel()->index(
                  0, 0, materialized_source_root);
              if (source_first_child.isValid())
                first_child = proxy_model->mapFromSource(source_first_child);
            }
          }
        }

        if (!first_child.isValid())
          return false;

        tree->scrollTo(first_child);
        tree->doItemsLayout();
        tree->viewport()->update();
        return !tree->visualRect(first_child).isEmpty();
      },
      2000);
  int source_child_count = -1;
  if (proxy_model && materialized_source_root.isValid()) {
    source_child_count =
        proxy_model->sourceModel()->rowCount(materialized_source_root);
  }

  if (!first_child_visible) {
    const auto& visible_root = tree->rootIndex();
    const auto proxy_child_count = tree->model()->rowCount(visible_root);
    const auto first_child = tree->model()->index(0, 0, visible_root);
    const auto first_child_rect =
        first_child.isValid() ? tree->visualRect(first_child) : QRect{};
    const auto root_rect = tree->visualRect(visible_root);
    const auto viewport_size = tree->viewport()->size();

    ADD_FAILURE() << "Struct tree did not materialize visible rows before "
                  << "capture"
                  << " | proxy_child_count=" << proxy_child_count
                  << " | source_child_count=" << source_child_count
                  << " | viewport=" << viewport_size.width() << "x"
                  << viewport_size.height() << " | root_rect=" << root_rect.x()
                  << "," << root_rect.y() << " " << root_rect.width() << "x"
                  << root_rect.height()
                  << " | first_child_valid=" << first_child.isValid()
                  << " | first_child_rect=" << first_child_rect.x() << ","
                  << first_child_rect.y() << " " << first_child_rect.width()
                  << "x" << first_child_rect.height()
                  << " | tree_visible=" << tree->isVisible()
                  << " | viewport_visible=" << tree->viewport()->isVisible()
                  << " | dock_visible="
                  << (tree_dock ? tree_dock->isVisible() : true);
  }
  ASSERT_TRUE(first_child_visible);
  ASSERT_TRUE(WaitUntil([&] {
    const auto loading_suffix =
        QString::fromStdU16String(u"[" + Translate("Loading") + u"]");
    for (int row = 0; row < tree->model()->rowCount(tree->rootIndex()); ++row) {
      const auto index = tree->model()->index(row, 0, tree->rootIndex());
      if (index.data(Qt::DisplayRole).toString().contains(loading_suffix))
        return false;
    }
    return true;
  }));
  for (int i = 0; i < 10; ++i)
    QApplication::processEvents();

  QPixmap pixmap = GrabWhenSettled(qmain);
  pixmap.save(QString::fromStdString(output_image.string()));

  MainWindow::SetHideForTesting(true);
}

// The Overview landing cockpit: a fresh (page-less) profile under the reshell
// theme boots through the production seeding path — BaseMainWindow falls back
// to CreateInitialPage, which returns MakeOverviewPage under the theme — so
// the capture guards the reshell initial-page routing and the page's
// dominant-trend/alarm-strip split, not a hand-assembled page.
TEST_F(ScreenshotGenerator, CaptureOverviewPage) {
  const char* filename = "workbench-overview.png";
  if (GetScreenshotOptions().theme.empty())
    GTEST_SKIP() << "the Overview landing seeds only under the reshell theme";
  if (!ShouldCaptureScreenshot(filename))
    GTEST_SKIP() << filename << " not requested";

  MainWindow::SetHideForTesting(false);

  auto output_dir = GetOutputDir();
  std::filesystem::create_directories(output_dir);
  const auto output_image = output_dir / filename;

  // Deliberately no saved profile: the page-less boot is the state under test.
  WaitForAwaitable(executor_, app_.Start());
  ASSERT_TRUE(WaitForPendingNodeLoads(app_.node_service()));

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
  if (GetScreenshotOptions().theme.empty())
    GTEST_SKIP() << "the activity rail is reshell chrome, themed runs only";
  if (!ShouldCaptureScreenshot(kFilename))
    GTEST_SKIP() << kFilename << " not requested";

  MainWindow::SetHideForTesting(false);

  auto output_dir = GetOutputDir();
  std::filesystem::create_directories(output_dir);

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
  ASSERT_TRUE(WaitForPendingNodeLoads(app_.node_service()));
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

  const QPixmap frame = GrabWhenSettled(rail);
  ASSERT_FALSE(frame.isNull());
  ASSERT_TRUE(
      frame.save(QString::fromStdString((output_dir / kFilename).string())))
      << "could not write " << kFilename;
}

// Settings → Colour scheme, the operator-facing switch for the experimental UX
// themes. Captured in the *default* (untheme'd) run on purpose: the operator
// who needs this image is the one still on Classic, looking for how to turn the
// reshell on.
//
// The menu bar is model-driven and rebuilt on every aboutToShow, so this walks
// the same path a real click does — emit aboutToShow, let BuildMenu populate,
// then grab the populated submenu.
TEST_F(ScreenshotGenerator, CaptureSettingsDialog) {
  constexpr const char* kFilename = "settings-dialog.png";
  if (!ShouldCaptureScreenshot(kFilename))
    GTEST_SKIP() << kFilename << " not requested";
  if (!GetScreenshotOptions().theme.empty())
    GTEST_SKIP() << kFilename << " is captured untheme'd only";

  MainWindow::SetHideForTesting(false);

  const auto output_dir = GetOutputDir();
  std::filesystem::create_directories(output_dir);

  WaitForAwaitable(executor_, app_.Start());
  ASSERT_TRUE(WaitForPendingNodeLoads(app_.node_service()));
  for (int i = 0; i < 20; ++i)
    QApplication::processEvents();

  const auto& main_windows = app_.main_window_manager().main_windows();
  ASSERT_EQ(main_windows.size(), 1u);
  auto* qmain = dynamic_cast<QMainWindow*>(&main_windows.front());
  ASSERT_NE(qmain, nullptr);
  qmain->show();

  // The menu now carries one item that opens the dialog, so the reachability
  // this used to guard on the Colour scheme submenu is guarded here instead:
  // Colour scheme is how the operator turns the reshell on, and it must not
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

  QAction* open_dialog = nullptr;
  const auto item_title = QString::fromStdU16String(Translate("Settings..."));
  for (QAction* action : settings_menu->actions()) {
    if (!action->isSeparator() && action->text() == item_title)
      open_dialog = action;
  }
  ASSERT_NE(open_dialog, nullptr)
      << "Settings has no Settings... item - the preferences dialog, and with "
         "it the appearance switch, is unreachable";

  // Build the dialog the same way the menu item and the rail's pinned utility
  // both do, rather than re-deriving its contents here.
  auto* qmain_window = dynamic_cast<MainWindow*>(&main_windows.front());
  ASSERT_NE(qmain_window, nullptr);
  auto* menu_model =
      dynamic_cast<MainMenuModel*>(qmain_window->main_menu_model());
  ASSERT_NE(menu_model, nullptr);
  SettingsDialog dialog{qmain, menu_model->settings_model()};
  dialog.ensurePolished();
  dialog.adjustSize();
  dialog.show();
  for (int i = 0; i < 10; ++i)
    QApplication::processEvents();

  // Guard the contents, not just that a dialog exists: an empty form would
  // still render a plausible-looking image. Colour scheme is the row that
  // matters most, so it is named rather than counted.
  const QList<QComboBox*> combos = dialog.findChildren<QComboBox*>();
  const QList<QCheckBox*> checks = dialog.findChildren<QCheckBox*>();
  EXPECT_GE(combos.size(), 1) << "expected Language / Style / Colour scheme";
  EXPECT_GT(checks.size(), 0) << "expected the preference toggles";
  bool has_appearances = false;
  for (const QComboBox* combo : combos) {
    if (combo->count() == 5)
      has_appearances = true;
  }
  EXPECT_TRUE(has_appearances)
      << "no row offers Classic plus the four appearances";

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

  QPixmap dialog_pixmap = GrabWhenSettled(&dialog);
  ASSERT_FALSE(dialog_pixmap.isNull());
  dialog_pixmap.save(QString::fromStdString((output_dir / kFilename).string()));
}

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

// Returns the fixture's single `MainWindow` as a `QMainWindow`, shown and
// pumped so its menu bar and command toolbar are live. `app_.Start()` must
// already have run.
QMainWindow* ShowMainWindowForMenuCapture(ClientApplication& app) {
  const auto& main_windows = app.main_window_manager().main_windows();
  if (main_windows.size() != 1u) {
    ADD_FAILURE() << "expected exactly one main window, got "
                  << main_windows.size();
    return nullptr;
  }
  auto* qmain = dynamic_cast<QMainWindow*>(&main_windows.front());
  if (!qmain) {
    ADD_FAILURE() << "the main window is not a QMainWindow";
    return nullptr;
  }
  qmain->resize(1920, 1080);
  qmain->show();
  for (int i = 0; i < 20; ++i)
    QApplication::processEvents();
  return qmain;
}

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
// `GetOutputDir() / filename`.
void SaveMenuCapture(QMenu* menu, const char* filename) {
  menu->ensurePolished();
  menu->adjustSize();
  for (int i = 0; i < 10; ++i)
    QApplication::processEvents();

  const auto output_dir = GetOutputDir();
  std::filesystem::create_directories(output_dir);
  const QPixmap pixmap = GrabWhenSettled(menu);
  ASSERT_FALSE(pixmap.isNull()) << filename << " grabbed an empty pixmap";
  ASSERT_TRUE(
      pixmap.save(QString::fromStdString((output_dir / filename).string())))
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
  if (!GetScreenshotOptions().theme.empty())
    GTEST_SKIP() << kFilename << " is captured untheme'd only";

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
  ASSERT_TRUE(WaitForPendingNodeLoads(app_.node_service()));

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

  SaveMenuCapture(menu, kFilename);
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
  if (!GetScreenshotOptions().theme.empty())
    GTEST_SKIP() << kFilename << " is captured untheme'd only";

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
  ASSERT_TRUE(WaitForPendingNodeLoads(app_.node_service()));

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

  SaveMenuCapture(menu, kFilename);
}

// Regression test for a stack overflow that fires during `app_.Start()`
// when a page containing a Struct (tree) window is loaded on top of the
// in-memory address space.
//
// The fixture wires the real `v1::NodeServiceImpl` and
// `NodeServiceTreeImpl` over synchronous Local* services that complete
// fetch callbacks in the caller's stack frame. During boot the tree
// model opens the root; each `ConfigurationTreeNode` ctor calls
// `node_.Fetch(NodeOnly)`; the fetch completes synchronously, fires
// `OnNodeChildrenChanged`, which re-enters `UpdateChildTreeNodes`, which
// creates more `ConfigurationTreeNode` children, each calling `Fetch()`
// again — and so on until the stack runs out. In production (gRPC)
// fetches return async over a socket, so the chain stays shallow and
// the bug never surfaces.
//
// This test is the smallest repro: only a Struct window on the page,
// no screenshots, no dialogs. The bug fix should let `app_.Start()`
// return normally and leave one main window open.
TEST_F(ScreenshotGenerator, BootWithStructPageDoesNotOverflowStack) {
  {
    Profile profile;
    Page page;
    page.AddWindow(WindowDefinition{"Struct"});
    profile.AddPage(page);
    profile.Save();
  }

  WaitForAwaitable(executor_, app_.Start());

  // Let the initial address-space fetch cascade complete.
  for (int i = 0; i < 20; ++i)
    QApplication::processEvents();

  // If the fetch cascade overflowed the stack the process would have
  // aborted before this line — reaching here means the recursion is
  // bounded.
  EXPECT_EQ(app_.main_window_manager().main_windows().size(), 1u);
}

// Verifies the event journal's Area filter populates at runtime: the same
// enumeration the filter bar drives (`BrowseEventAreas`), run against the real
// `v1::NodeServiceImpl` over the fixture address space, returns the operator's
// top-level area groupings and drops leaf data items.
TEST_F(ScreenshotGenerator, EventFilterBarEnumeratesAreas) {
  WaitForAwaitable(executor_, app_.Start());
  ASSERT_TRUE(WaitForPendingNodeLoads(app_.node_service()));

  NodeService& node_service = app_.node_service();
  std::vector<EventAreaEntry> areas =
      WaitForAwaitable(executor_, BrowseEventAreas(node_service));

  // The Area dropdown is populated from this list — it must not be empty.
  ASSERT_FALSE(areas.empty());

  // Every enumerated area is a named object grouping, never a leaf data item —
  // that is the level the operator filters the journal by.
  for (const EventAreaEntry& area : areas) {
    EXPECT_FALSE(area.name.empty());
    NodeRef node = node_service.GetNode(area.node_id);
    EXPECT_FALSE(IsInstanceOf(node, scada::data_items::id::DataItemType))
        << "an area must not be a leaf data item";
  }

  // The DataItems root also holds loose top-level data items; the enumeration
  // partitions its Organizes children exactly into areas + excluded leaves.
  std::vector<NodeRef> children = node_service.GetTargets(
      scada::data_items::id::DataItems, scada::id::Organizes, /*forward=*/true);
  size_t leaf_count = 0;
  for (NodeRef& child : children) {
    if (IsInstanceOf(child, scada::data_items::id::DataItemType))
      ++leaf_count;
  }
  EXPECT_EQ(areas.size() + leaf_count, children.size());
}

// Verifies the Transmission view's destination rail populates at runtime: the
// same walk the rail drives (`BrowseTransmissionDevices`), run against the
// real `v1::NodeServiceImpl` over the fixture address space, finds every
// transmission-capable fixture device with its rule count — the plain Modbus
// device (a valid, rule-less destination) and both retransmission devices.
TEST_F(ScreenshotGenerator, DestinationRailEnumeratesTransmissionDevices) {
  WaitForAwaitable(executor_, app_.Start());
  ASSERT_TRUE(WaitForPendingNodeLoads(app_.node_service()));

  NodeService& node_service = app_.node_service();
  std::vector<TransmissionDeviceEntry> devices = WaitForAwaitable(
      executor_, BrowseTransmissionDevices(
                     node_service.GetNode(scada::devices::id::Devices)));

  ASSERT_EQ(devices.size(), 3u);
  EXPECT_EQ(devices[0].node_id, NodeIdFromScadaString("TS.104"));
  EXPECT_EQ(devices[0].rule_count, 0);
  EXPECT_EQ(devices[1].node_id, NodeIdFromScadaString("TS.702"));
  EXPECT_EQ(devices[1].rule_count, 4);
  EXPECT_EQ(devices[2].node_id, NodeIdFromScadaString("TS.703"));
  EXPECT_EQ(devices[2].rule_count, 2);
  for (const TransmissionDeviceEntry& device : devices)
    EXPECT_FALSE(device.name.empty());
}

// The fixture accounts that carry `right`, by display name — the same
// projection `fixture_builder.cpp` makes when it creates the identity mapping
// rules. Derived from the fixture JSON rather than written out, both because
// the names are Russian (which may not appear in a client string literal) and
// so the expectation cannot drift from the fixture it describes.
std::vector<std::u16string> FixtureAccountsWith(scada::AccessRight right) {
  namespace sec = scada::security::id;
  const std::string rights_key =
      NodeIdToScadaString(sec::UserType_AccessRights);

  std::vector<std::u16string> names;
  for (const auto& node : g_config.json.at("nodes").as_array()) {
    const auto& object = node.as_object();
    const auto* type = object.if_contains("type_definition");
    if (!type || !type->is_string() ||
        NodeIdFromScadaString(std::string_view{type->as_string()}) !=
            sec::UserType) {
      continue;
    }
    const auto* display = object.if_contains("display_name");
    if (!display || !display->is_string())
      continue;

    std::int64_t access_rights = 0;
    if (const auto* properties = object.if_contains("properties");
        properties && properties->is_object()) {
      if (const auto* value = properties->as_object().if_contains(rights_key);
          value && value->is_number()) {
        access_rights = value->to_number<std::int64_t>();
      }
    }
    if (access_rights & scada::AccessRightBit(right))
      names.push_back(UtfConvert<char16_t>(std::string{display->as_string()}));
  }
  return names;
}

// Role membership must survive the real node model, not just a static fake.
//
// `ReadRoleMemberships` resolves each rule's Criteria through the TYPE's
// property declarations, so the type chain has to be fetched first. It was
// not, and against the real address space both property lookups returned a
// null NodeRef — every Role came back with no members, which is what
// `roles.png` and the managed `users-admin.png` documented. The module's own
// unit tests could not see it: a StaticNodeService resolves the declaration
// whether or not anything fetched the type.
TEST_F(ScreenshotGenerator, RolesEnumerateTheirMembers) {
  WaitForAwaitable(executor_, app_.Start());
  ASSERT_TRUE(WaitForPendingNodeLoads(app_.node_service()));

  const std::optional<std::vector<RoleMembership>> roles = WaitForAwaitable(
      executor_, ReadRoleMemberships(executor_, app_.node_service(),
                                     authenticated_attribute_service_));
  ASSERT_TRUE(roles.has_value());

  const auto members_of = [&roles](scada::WellKnownRole role) {
    const scada::NodeId id = scada::WellKnownRoleId(role);
    auto i = std::ranges::find(*roles, id, &RoleMembership::node_id);
    return i != roles->end() ? i->members : std::vector<std::u16string>{};
  };

  const std::vector<std::u16string> control =
      FixtureAccountsWith(scada::AccessRight::kControl);
  const std::vector<std::u16string> configure =
      FixtureAccountsWith(scada::AccessRight::kConfigure);
  ASSERT_FALSE(control.empty());
  ASSERT_FALSE(configure.empty());

  EXPECT_THAT(members_of(scada::WellKnownRole::kOperator),
              testing::UnorderedElementsAreArray(control));
  EXPECT_THAT(members_of(scada::WellKnownRole::kConfigureAdmin),
              testing::UnorderedElementsAreArray(configure));
}

// The limits dialog draws whatever the target node's four limit-band
// properties hold, and nothing else — so a node without them captures a
// dialog whose every field is blank. `limits.png` shipped that way against
// the manual, which shows all four populated.
//
// Locking the values down here rather than only in the fixture: the capture
// pipeline has no assertion that would notice the fields going empty again
// (check_screenshots.py verifies a PNG exists with the right dimensions,
// which an empty dialog satisfies), and the properties are easy to drop while
// editing the fixture for some other capture. Driving the real LimitModel,
// not the raw property reads, keeps the residency requirement in scope: the
// bands are property children, and an unfetched node reads them all as null.
TEST_F(ScreenshotGenerator, LimitDialogNodeCarriesItsBands) {
  WaitForAwaitable(executor_, app_.Start());
  ASSERT_TRUE(WaitForPendingNodeLoads(app_.node_service()));

  const scada::NodeId node_id = g_config.dialog_analog_node_id;
  ASSERT_TRUE(scada::screenshot_generator::FetchNodesResident(
      app_.node_service(), std::span<const scada::NodeId>{&node_id, 1}));

  NullTaskManager task_manager;
  LimitModel model{
      LimitDialogContext{app_.node_service().GetNode(node_id), task_manager}};

  EXPECT_FALSE(model.GetSourceTitle().empty())
      << "the dialog's source title is the node display name";

  const LimitModel::Limits limits = model.GetLimits();
  EXPECT_EQ(limits.hihi, u"90");
  EXPECT_EQ(limits.hi, u"70");
  EXPECT_EQ(limits.lo, u"-10");
  EXPECT_EQ(limits.lolo, u"-25");
}

// The control dialog's condition row exists only when the target node carries
// a DataItemType_OutputCondition formula, and it reads "Satisfied" only when
// that formula evaluates truthy over live data. Both are fixture properties
// and both are invisible to the PNG comparison — the row's absence just makes
// the dialog shorter — so assert them here: without this the fixture could
// silently lose the property and ti-remote-control-enabled.png would go back
// to missing the row the docs original shows.
TEST_F(ScreenshotGenerator, ControlDialogNodeCarriesItsOutputCondition) {
  WaitForAwaitable(executor_, app_.Start());
  ASSERT_TRUE(WaitForPendingNodeLoads(app_.node_service()));

  const scada::NodeId node_id = g_config.dialog_analog_node_id;
  ASSERT_TRUE(scada::screenshot_generator::FetchNodesResident(
      app_.node_service(), std::span<const scada::NodeId>{&node_id, 1}));

  Profile profile;
  auto model = std::make_shared<WriteModel>(
      WriteContext{.executor_ = executor_,
                   .timed_data_service_ = app_.timed_data_service(),
                   .node_id_ = node_id,
                   .profile_ = profile,
                   .manual_ = false});

  EXPECT_TRUE(model->has_condition())
      << "the fixture node lost its DataItemType_OutputCondition; the control "
         "dialog would render without its condition row";

  // The condition subscribes through TimedDataService like any other item, so
  // its first value arrives on a later turn of the loop.
  scada::screenshot_generator::PumpEventLoopFor(std::chrono::milliseconds{500});

  EXPECT_TRUE(model->IsConditionOk())
      << "the condition formula must evaluate truthy over the fixture data, so "
         "the dialog reads Satisfied and its OK button is enabled";
}

// The mirror of the test above, for the capture that shows the dialog refusing
// the command. ti-remote-control-disabled.png is the same `write-remote` kind
// over a different node — the fixture cannot make one node's condition read
// both ways in a single run — so the whole capture rests on the dialog spec's
// `node` override reaching a node whose formula is falsy. Neither half is
// visible to check_screenshots.py, which only asserts the PNG exists: an
// override silently dropped, or a formula that quietly became true, would
// still produce a file, and it would be the enabled dialog under the disabled
// dialog's name.
TEST_F(ScreenshotGenerator, DisabledControlDialogNodeConditionIsUnsatisfied) {
  // Read the spec straight out of the fixture rather than from
  // g_config.dialogs: that vector is filtered by the managed-image gate and by
  // --only, and the themed ctest run passes --only.
  const boost::json::value* dialog = nullptr;
  for (const auto& js : g_config.json.at("dialogs").as_array()) {
    if (js.at("filename").as_string() == "ti-remote-control-disabled.png")
      dialog = &js;
  }
  ASSERT_NE(dialog, nullptr)
      << "the fixture lost the disabled control-dialog capture";

  const auto* node = dialog->as_object().if_contains("node");
  ASSERT_NE(node, nullptr)
      << "the disabled variant needs its own node: sharing the fixture-wide "
         "dialog_analog_node_id renders the enabled dialog instead";
  const scada::NodeId node_id =
      NodeIdFromScadaString(std::string_view(node->as_string()));
  ASSERT_NE(node_id, g_config.dialog_analog_node_id);

  WaitForAwaitable(executor_, app_.Start());
  ASSERT_TRUE(WaitForPendingNodeLoads(app_.node_service()));
  ASSERT_TRUE(scada::screenshot_generator::FetchNodesResident(
      app_.node_service(), std::span<const scada::NodeId>{&node_id, 1}));

  Profile profile;
  auto model = std::make_shared<WriteModel>(
      WriteContext{.executor_ = executor_,
                   .timed_data_service_ = app_.timed_data_service(),
                   .node_id_ = node_id,
                   .profile_ = profile,
                   .manual_ = false});

  EXPECT_TRUE(model->has_condition())
      << "without a DataItemType_OutputCondition the dialog renders no "
         "condition row at all, which is neither variant";

  scada::screenshot_generator::PumpEventLoopFor(std::chrono::milliseconds{500});

  EXPECT_FALSE(model->IsConditionOk())
      << "the condition formula must evaluate falsy over the fixture data, so "
         "the dialog reads Not satisfied and its OK button is disabled";
}

TEST_F(ScreenshotGenerator, CaptureDialogs) {
  auto output_dir = GetOutputDir();
  std::filesystem::create_directories(output_dir);

  // Start the app so the real TimedDataServiceImpl is wired up; the
  // WriteDialog family reads current values, formula titles, and
  // engineering units through it.
  WaitForAwaitable(executor_, app_.Start());
  ASSERT_TRUE(WaitForPendingNodeLoads(app_.node_service()));
  for (int i = 0; i < 20; ++i)
    QApplication::processEvents();

  Profile profile;
  DialogEnvironment env{.executor = executor_,
                        .node_service = &app_.node_service(),
                        .timed_data_service = &app_.timed_data_service(),
                        .profile = &profile,
                        .dialog_analog_node_id = g_config.dialog_analog_node_id,
                        .login_user_list = g_config.login_user_list};

  int captured = 0;
  const bool themed = !GetScreenshotOptions().theme.empty();
  for (const auto& spec : g_config.dialogs) {
    // A themed-only spec is the reshell twin of a legacy capture (see
    // workbench-login.png): rendering it without the theme would save the
    // legacy dialog under the reshell name.
    if (spec.themed_only && !themed)
      continue;
    if (CaptureDialog(spec, env))
      ++captured;
  }

  std::cout << "Captured " << captured << "/" << g_config.dialogs.size()
            << " dialogs to " << output_dir.string() << std::endl;
}
