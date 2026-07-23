#include "app/qt/e2e_test_support.h"
#include "base/time/time_wire_codec.h"

#include "app/client_application.h"
#include "aui/qt/message_loop_qt.h"
#include "base/any_executor.h"
#include "base/any_executor_timer.h"
#include "base/awaitable.h"
#include "base/callback_awaitable.h"
#include "base/e2e_test_hooks.h"
#include "base/time_range.h"
#include "base/utf_convert.h"
#include "common/formula_util.h"
#include "configuration/devices/hardware_tree_view.h"
#include "configuration/objects/object_tree_view.h"
#include "controller/command_handler.h"
#include "controller/window_info.h"
#include "export/csv/csv_export_util.h"
#include "main_window/main_window.h"
#include "main_window/main_window_manager.h"
#include "main_window/opened_view/opened_view.h"
#include "model/namespaces.h"
#include "model/node_id_util.h"
#include "modules/timed_data/timed_data_controller.h"
#include "profile/page.h"
#include "profile/profile.h"
#include "profile/window_definition.h"
#include "profile/window_definition_util.h"
#include "resources/common_resources.h"
#include "scada/status.h"
#include "timed_data/timed_data_spec.h"

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <variant>
#include <vector>

using namespace std::chrono_literals;

namespace client {
namespace {

struct OperatorUseCaseSmokeState {
  bool ok = true;
  std::vector<std::string> lines;
};

void AddSmokeResult(const std::shared_ptr<OperatorUseCaseSmokeState>& state,
                    std::string_view id,
                    bool ok,
                    std::string detail) {
  state->ok = state->ok && ok;
  state->lines.emplace_back(std::string{id} + (ok ? " ok " : " failure ") +
                            std::move(detail));
}

void WriteOperatorUseCaseSmokeReport(const std::filesystem::path& path,
                                     const OperatorUseCaseSmokeState& state) {
  if (path.empty())
    return;

  std::error_code ec;
  if (path.has_parent_path())
    std::filesystem::create_directories(path.parent_path(), ec);

  std::ofstream output{path, std::ios::binary | std::ios::trunc};
  if (!output)
    return;

  output << "operator-use-cases: " << (state.ok ? "ok" : "failure") << "\n";
  for (const auto& line : state.lines)
    output << line << "\n";
}

void WriteObjectViewValuesReport(const std::filesystem::path& path,
                                 bool ok,
                                 std::string_view detail) {
  if (path.empty())
    return;

  std::error_code ec;
  if (path.has_parent_path())
    std::filesystem::create_directories(path.parent_path(), ec);

  std::ofstream output{path, std::ios::binary | std::ios::trunc};
  if (!output)
    return;

  output << "object-view-values: " << (ok ? "ok" : "failure") << "\n";
  output << detail << "\n";
}

void WriteObjectTreeLabelsReport(const std::filesystem::path& path,
                                 bool ok,
                                 const std::vector<std::u16string>& labels,
                                 std::string_view detail) {
  if (path.empty())
    return;

  std::error_code ec;
  if (path.has_parent_path())
    std::filesystem::create_directories(path.parent_path(), ec);

  std::ofstream output{path, std::ios::binary | std::ios::trunc};
  if (!output)
    return;

  output << "object-tree-labels: " << (ok ? "ok" : "failure") << "\n";
  output << detail << "\n";
  for (size_t i = 0; i < labels.size(); ++i)
    output << "label[" << i << "]=" << UtfConvert<char>(labels[i]) << "\n";
}

void WriteHardwareTreeDevicesReport(
    const std::filesystem::path& path,
    bool ok,
    const std::vector<HardwareTreeDeviceForTesting>& devices,
    std::string_view detail) {
  if (path.empty())
    return;

  std::error_code ec;
  if (path.has_parent_path())
    std::filesystem::create_directories(path.parent_path(), ec);

  std::ofstream output{path, std::ios::binary | std::ios::trunc};
  if (!output)
    return;

  output << "hardware-tree-devices: " << (ok ? "ok" : "failure") << "\n";
  output << detail << "\n";
  for (size_t i = 0; i < devices.size(); ++i) {
    output << "device[" << i << "].protocol=" << devices[i].protocol << "\n";
    output << "device[" << i << "].label=" << UtfConvert<char>(devices[i].label)
           << "\n";
    output << "device[" << i
           << "].active=" << (devices[i].active ? "true" : "false") << "\n";
    output << "device[" << i << "].state=" << devices[i].state << "\n";
  }
}

void WriteProfileSaveReport(const std::filesystem::path& path,
                            bool ok,
                            scada::Status status,
                            int page_id,
                            std::u16string_view page_title) {
  if (path.empty())
    return;

  std::error_code ec;
  if (path.has_parent_path())
    std::filesystem::create_directories(path.parent_path(), ec);

  std::ofstream output{path, std::ios::binary | std::ios::trunc};
  if (!output)
    return;

  output << "profile-save: " << (ok ? "ok" : "failure") << "\n";
  output << "status=" << ToString(status.code()) << "\n";
  output << "page-id=" << page_id << "\n";
  output << "page-title=" << UtfConvert<char>(page_title) << "\n";
}

MainWindow* GetFirstMainWindow(ClientApplication& app) {
  for (auto& main_window : app.main_window_manager().main_windows())
    return &main_window;
  return nullptr;
}

ObjectTreeView* FindObjectTreeView(ClientApplication& app) {
  auto* main_window = GetFirstMainWindow(app);
  if (!main_window)
    return nullptr;

  for (auto* opened_view : main_window->opened_views()) {
    if (opened_view->GetWindowInfo().name != std::string_view{"Struct"})
      continue;

    return dynamic_cast<ObjectTreeView*>(&opened_view->controller());
  }

  return nullptr;
}

HardwareTreeView* FindHardwareTreeView(ClientApplication& app) {
  auto* main_window = GetFirstMainWindow(app);
  if (!main_window)
    return nullptr;

  for (auto* opened_view : main_window->opened_views()) {
    if (opened_view->GetWindowInfo().name != std::string_view{"Subsystems"})
      continue;

    return dynamic_cast<HardwareTreeView*>(&opened_view->controller());
  }

  return nullptr;
}

// Exports the timed-data view's current rows to `path` as CSV, reusing the same
// ExportToCsv writer the ID_EXPORT_CSV command invokes (minus its interactive
// save-file dialog). The CSV is a header row plus one row per historical
// sample, so the number of newlines equals the number of samples the view read.
void WriteTimedDataCsvReport(TimedDataController& controller,
                             const std::filesystem::path& path) {
  if (path.empty())
    return;

  std::error_code ec;
  if (path.has_parent_path())
    std::filesystem::create_directories(path.parent_path(), ec);

  auto export_data = controller.GetExportData();
  if (auto* table = std::get_if<ExportModel::TableExportData>(&export_data))
    ExportToCsv(*table, CsvExportParams{}, path);
}

TimedDataController* FindTimedDataView(ClientApplication& app) {
  auto* main_window = GetFirstMainWindow(app);
  if (!main_window)
    return nullptr;

  for (auto* opened_view : main_window->opened_views()) {
    if (opened_view->GetWindowInfo().name != std::string_view{"TimeVal"})
      continue;

    return dynamic_cast<TimedDataController*>(&opened_view->controller());
  }

  return nullptr;
}

Awaitable<void> Delay(AnyExecutor executor, std::chrono::milliseconds delay) {
  co_await CallbackToAwaitable<>(
      executor, [executor, delay](auto done) mutable {
        PostDelayedTask(executor, delay,
                        [done = std::move(done)]() mutable { done(); });
      });
}

Awaitable<void> RunObjectViewValuesCheckAsync(
    ObjectViewValuesCheckContext context,
    std::filesystem::path report_path) {
  const auto deadline = std::chrono::steady_clock::now() + context.timeout;
  do {
    if (auto value_text = context.get_first_value_text()) {
      WriteObjectViewValuesReport(report_path, true, "value text present");
      co_return;
    }

    if (std::chrono::steady_clock::now() >= deadline)
      break;

    co_await Delay(context.executor, context.poll_interval);
  } while (true);

  WriteObjectViewValuesReport(report_path, false,
                              "timed out waiting for value text");
  co_return;
}

bool IsObjectTreeLabelsReady(const std::vector<std::u16string>& labels) {
  if (labels.size() != 4)
    return false;
  return std::ranges::all_of(labels, [](const auto& label) {
    return !label.empty() && label.find(u"[") == std::u16string::npos;
  });
}

Awaitable<void> RunObjectTreeLabelsCheckAsync(
    ObjectTreeLabelsCheckContext context,
    std::filesystem::path report_path) {
  const auto deadline = std::chrono::steady_clock::now() + context.timeout;
  std::vector<std::u16string> labels;
  // A ready-looking path is only accepted once it has repeated unchanged for
  // settle_duration; anything captured mid-FetchMore keeps changing and resets
  // the window. See ObjectTreeLabelsCheckContext::settle_duration.
  std::vector<std::u16string> candidate;
  std::chrono::steady_clock::time_point candidate_since;
  do {
    labels = context.get_expanded_labels();
    if (IsObjectTreeLabelsReady(labels)) {
      const auto now = std::chrono::steady_clock::now();
      if (labels != candidate) {
        candidate = labels;
        candidate_since = now;
      }
      if (now - candidate_since >= context.settle_duration) {
        WriteObjectTreeLabelsReport(report_path, true, labels,
                                    "expanded first rendered path (settled)");
        co_return;
      }
    } else {
      candidate.clear();
    }

    if (std::chrono::steady_clock::now() >= deadline)
      break;

    co_await Delay(context.executor, context.poll_interval);
  } while (true);

  WriteObjectTreeLabelsReport(
      report_path, false, labels,
      "timed out waiting for rendered labels to settle");
  co_return;
}

class HardwareTreeDevicesCheck final
    : public std::enable_shared_from_this<HardwareTreeDevicesCheck> {
 public:
  HardwareTreeDevicesCheck(ClientApplication& app,
                           AnyExecutor executor,
                           std::filesystem::path report_path)
      : app_{app},
        executor_{std::move(executor)},
        report_path_{std::move(report_path)},
        // Client-side capture budget for devices to browse AND come online.
        // Under full-suite load the device tiers need well past 30s to
        // establish their links, so poll longer before writing a failure
        // report (the matching test-side wait is strictly larger — see
        // kHardwareTreeDevicesTimeout).
        deadline_{std::chrono::steady_clock::now() + 60s} {}

  Awaitable<void> RunAsync() {
    auto* main_window = GetFirstMainWindow(app_);
    const auto* window_info = FindWindowInfoByName("Subsystems");
    if (!main_window || !window_info) {
      WriteHardwareTreeDevicesReport(report_path_, false, {},
                                     "hardware tree window missing");
      co_return;
    }

    auto* opened_view =
        co_await main_window->OpenView(WindowDefinition{*window_info},
                                       /*activate=*/true);
    if (!opened_view) {
      WriteHardwareTreeDevicesReport(report_path_, false, {},
                                     "hardware tree failed to open");
      co_return;
    }

    StartProtocolActivity();
    co_await PollAsync();
  }

 private:
  static bool HasDevice(
      const std::vector<HardwareTreeDeviceForTesting>& devices,
      std::string_view protocol) {
    return std::ranges::any_of(devices, [&](const auto& device) {
      return device.protocol == protocol;
    });
  }

  // The settle condition. Requires every expanded device — of all three
  // protocols — to have a RESOLVED runtime status (anything but Unknown), not
  // merely one active device per protocol. The earlier one-per-protocol rule
  // captured the report the instant the first device of each protocol came up,
  // i.e. before its siblings settled, so a device whose runtime status never
  // reached the client (the device-online routing regression this guards
  // against) went unnoticed as long as one sibling was up. The test asserts on
  // Unknown, not Online, deliberately: a device that never routes its status
  // through the proxy reads Unknown, whereas a device that is genuinely not
  // connected reports Offline — a real value that DID route (e.g. the IEC60870
  // server-side device has no peer and settles Offline). "Every device
  // resolved" therefore catches the routing gap without asserting connectivity
  // the loopback fixture does not give every device.
  static bool DeviceResolved(const HardwareTreeDeviceForTesting& device) {
    return device.state != "Unknown";
  }

  static bool AllProtocolsResolved(
      const std::vector<HardwareTreeDeviceForTesting>& devices) {
    return HasDevice(devices, "MODBUS") && HasDevice(devices, "IEC60870") &&
           HasDevice(devices, "IEC61850") &&
           std::ranges::all_of(devices, DeviceResolved);
  }

  void StartProtocolActivity() {
    activation_specs_.clear();

    activation_specs_.emplace_back(
        app_.timed_data_service(),
        MakeNestedNodeId(
            scada::NodeId{2, scada::NamespaceIndexes::MODBUS_DEVICES},
            "BOOL:1"));
    activation_specs_.emplace_back(
        app_.timed_data_service(),
        MakeNestedNodeId(
            scada::NodeId{2, scada::NamespaceIndexes::IEC60870_DEVICE}, "111"));
  }

  Awaitable<void> PollAsync() {
    while (std::chrono::steady_clock::now() < deadline_) {
      if (auto* hardware_tree_view = FindHardwareTreeView(app_)) {
        auto devices = hardware_tree_view->GetExpandedDevicesForTesting();
        // Require the device set to have stopped growing before accepting an
        // all-resolved snapshot, so a tree still populating (only its first,
        // already-resolved device visible) is not mistaken for "every device
        // resolved". Two consecutive polls of equal, non-zero size settle it.
        const bool stable =
            !devices.empty() && devices.size() == last_device_count_;
        last_device_count_ = devices.size();
        if (stable && AllProtocolsResolved(devices)) {
          WriteHardwareTreeDevicesReport(report_path_, true, devices,
                                         "expanded hardware tree devices");
          co_return;
        }
      }
      co_await Delay(executor_, 100ms);
    }

    std::vector<HardwareTreeDeviceForTesting> devices;
    if (auto* hardware_tree_view = FindHardwareTreeView(app_))
      devices = hardware_tree_view->GetExpandedDevicesForTesting();
    WriteHardwareTreeDevicesReport(report_path_, false, devices,
                                   "timed out waiting for online devices");
  }

  ClientApplication& app_;
  AnyExecutor executor_;
  const std::filesystem::path report_path_;
  const std::chrono::steady_clock::time_point deadline_;
  std::vector<TimedDataSpec> activation_specs_;
  // Previous poll's device count, for the "set has stopped growing" gate.
  std::size_t last_device_count_ = 0;
};

Awaitable<OperatorUseCaseSmokeResult> OpenOperatorWindowAsync(
    ClientApplication& app,
    AnyExecutor executor,
    std::string window_type) {
  auto* main_window = GetFirstMainWindow(app);
  if (!main_window) {
    co_return OperatorUseCaseSmokeResult{.ok = false,
                                         .detail = "main window missing"};
  }

  if (const auto* window_info = FindWindowInfoByName(window_type)) {
    auto* opened_view =
        co_await main_window->OpenView(WindowDefinition{*window_info},
                                       /*activate=*/true);
    co_return OperatorUseCaseSmokeResult{
        .ok = opened_view != nullptr,
        .detail = opened_view ? "opened" : "open returned null"};
  } else {
    co_return OperatorUseCaseSmokeResult{
        .ok = false, .detail = "window type not registered"};
  }
}

Awaitable<OperatorUseCaseSmokeResult> CheckContextMenuCommandsAsync(
    ClientApplication& app,
    AnyExecutor executor,
    std::string window_type,
    std::vector<unsigned> command_ids) {
  auto result =
      co_await OpenOperatorWindowAsync(app, executor, std::move(window_type));
  if (!result.ok)
    co_return result;

  co_await Delay(executor, 100ms);

  auto* main_window = GetFirstMainWindow(app);
  if (!main_window) {
    co_return OperatorUseCaseSmokeResult{.ok = false,
                                         .detail = "main window missing"};
  }

  std::string detail = "context-menu";
  bool ok = true;
  for (unsigned command_id : command_ids) {
    bool available =
        main_window->IsContextMenuCommandAvailableForTesting(command_id);
    ok = ok && available;
    detail += available ? " command " : " missing-command ";
    detail += std::to_string(command_id);
  }

  co_return OperatorUseCaseSmokeResult{.ok = ok, .detail = std::move(detail)};
}

std::vector<OperatorUseCaseSmokeCheck> MakeOperatorUseCaseSmokeChecks() {
  return {
      {.id = "UC-1",
       .description = "monitor live values",
       .open_window_types = {"Log"}},
      {.id = "UC-2",
       .description = "visualise time-series on a graph",
       .open_window_types = {"Graph"}},
      {.id = "UC-3",
       .description = "view tables summaries and sheets",
       .open_window_types = {"Table", "Summ", "CusTable"}},
      {.id = "UC-4",
       .description = "acknowledge events and alarms",
       .open_window_types = {"Event"}},
      {.id = "UC-5",
       .description = "browse event journals",
       .open_window_types = {"EventJournal"}},
      {.id = "UC-6",
       .description = "watch a custom spreadsheet",
       .open_window_types = {"CusTable"}},
      {.id = "UC-7",
       .description = "issue control commands",
       .registered_selection_commands = {ID_WRITE, ID_WRITE_MANUAL}},
      {.id = "UC-8",
       .description = "manage favourites and portfolios",
       .open_window_types = {"Favorites", "Portfolio"}},
      {.id = "UC-9",
       .description = "print or export the active view",
       .context_menu_commands = {{"Table", {ID_PRINT, ID_EXPORT_CSV}},
                                 {"EventJournal", {ID_PRINT, ID_EXPORT_CSV}}},
       .printable_window_types = {"Table", "EventJournal"}},
      {.id = "UC-10",
       .description = "browse server files",
       .open_window_types = {"FileSystemView"}},
      {.id = "UC-11",
       .description = "view Modus and Vidicon schematics",
       .registered_window_types = {"Modus", "VidiconDisplay"},
       .optional_when_unavailable = true},
      {.id = "UC-12",
       .description = "edit device parameters limits and aliases",
       .registered_window_types = {"NewProps", "Params"},
       .registered_selection_commands = {ID_EDIT_LIMITS}},
      {.id = "UC-13",
       .description = "bulk-create data items",
       .registered_window_types = {"TableEditor"}},
      {.id = "UC-14",
       .description = "export or import configuration",
       .registered_global_commands = {ID_EXPORT_CONFIGURATION_TO_EXCEL,
                                      ID_IMPORT_CONFIGURATION_FROM_EXCEL}},
      {.id = "UC-15",
       .description = "inspect protocol traffic",
       .registered_selection_commands = {ID_DUMP_DEBUG_INFO}},
      {.id = "UC-16",
       .description = "save window layouts and profiles",
       .registered_global_commands = {ID_PAGE_NEW, ID_PAGE_RENAME,
                                      ID_PAGE_DELETE}},
      {.id = "UC-17",
       .description = "authenticate against a back-end",
       .main_window_commands = {ID_LOGOFF}},
      {.id = "UC-18",
       .description = "manage users and passwords",
       .registered_window_types = {"Users"},
       .registered_selection_commands = {ID_CHANGE_PASSWORD},
       .main_window_commands = {ID_USERS_VIEW}},
      {.id = "UC-19",
       .description = "configure transmission rules",
       .registered_window_types = {"Transmission"}},
  };
}

Awaitable<void> RunE2eOperatorUseCaseSmokeAsync(
    OperatorUseCaseSmokeContext context,
    std::filesystem::path report_path,
    std::vector<OperatorUseCaseSmokeCheck> checks) {
  auto state = std::make_shared<OperatorUseCaseSmokeState>();
  auto executor = context.executor;

  for (const auto& check : checks) {
    for (std::string_view window_type : check.open_window_types) {
      auto result = co_await context.open_window(window_type);
      AddSmokeResult(state, window_type, result.ok, std::move(result.detail));
    }

    bool ok = true;
    std::string detail{check.description};

    for (std::string_view window_type : check.registered_window_types) {
      bool registered = context.is_window_registered(window_type);
      ok = ok && registered;
      detail += registered ? " registered " : " missing ";
      detail += window_type;
    }

    for (unsigned command_id : check.registered_selection_commands) {
      bool registered = context.has_selection_command(command_id);
      ok = ok && registered;
      detail += registered ? " command " : " missing-command ";
      detail += std::to_string(command_id);
    }

    for (unsigned command_id : check.registered_global_commands) {
      bool registered = context.has_global_command(command_id);
      ok = ok && registered;
      detail += registered ? " global-command " : " missing-global-command ";
      detail += std::to_string(command_id);
    }

    for (unsigned command_id : check.main_window_commands) {
      bool available = context.has_main_window_command(command_id);
      ok = ok && available;
      detail +=
          available ? " main-window-command " : " missing-main-window-command ";
      detail += std::to_string(command_id);
    }

    for (const auto& context_menu : check.context_menu_commands) {
      auto result = co_await context.has_context_menu_commands(
          context_menu.window_type, context_menu.command_ids);
      ok = ok && result.ok;
      detail += result.ok ? " context-menu " : " missing-context-menu ";
      detail += std::string{context_menu.window_type};
      detail += " ";
      detail += std::move(result.detail);
    }

    for (std::string_view window_type : check.printable_window_types) {
      bool printable = context.is_window_printable(window_type);
      ok = ok && printable;
      detail += printable ? " printable " : " not-printable ";
      detail += window_type;
    }

    if (!ok && check.optional_when_unavailable) {
      ok = true;
      detail += " optional-unavailable";
    }

    AddSmokeResult(state, check.id, ok, std::move(detail));
  }

  WriteOperatorUseCaseSmokeReport(report_path, *state);
  co_return;
}

// Writes an empty report file so a hard failure (no view at all) still leaves a
// concrete, inspectable artifact instead of an absent file the harness can only
// observe as a wait timeout.
void WriteEmptyReport(const std::filesystem::path& path) {
  if (path.empty())
    return;
  std::error_code ec;
  if (path.has_parent_path())
    std::filesystem::create_directories(path.parent_path(), ec);
  std::ofstream output{path, std::ios::binary | std::ios::trunc};
}

Awaitable<void> RunHistoricalTimedDataCheckAsync(
    ClientApplication& app,
    AnyExecutor executor,
    std::filesystem::path report_path) {
  auto* main_window = GetFirstMainWindow(app);
  const auto* window_info = FindWindowInfoByName("TimeVal");
  if (main_window && window_info) {
    // Point the timed-data view at the historized, simulated analog item TIT.4.
    // TimedDataModel::Init reads the "Item"/"path" formula and defaults to a
    // Day window, so opening it triggers a historical HistoryRead through the
    // active backend (and, in the Cluster topology, through the proxy's
    // historian route).
    WindowDefinition definition{*window_info};
    definition.AddItem("Item").SetString(
        "path",
        MakeNodeIdFormula(scada::NodeId{4, scada::NamespaceIndexes::TIT}));
    // When the harness supplies a window end predating the client's launch,
    // pin the view to a fixed past window ending there. Live monitored-item
    // updates all carry source timestamps at or after the client connected
    // (the simulated item changes every ~1.5 s, so even the initial
    // notification is newer than the launch time); they fall outside the
    // window and TimedDataModel excludes them from the row count — rows can
    // then only come from a HistoryRead answered by the server-side history
    // store.
    if (auto end_time = GetE2eHistoricalTimedDataEndTime()) {
      const auto end = scada::base::DecodeWireTime(*end_time);
      SaveTimeRange(definition,
                    TimeRange{end - std::chrono::hours{1}, end});
    }
    co_await main_window->OpenView(std::move(definition), /*activate=*/true);
  }

  const auto deadline = std::chrono::steady_clock::now() + 30s;
  while (std::chrono::steady_clock::now() < deadline) {
    if (auto* view = FindTimedDataView(app);
        view && view->GetRowCountForTesting() > 0) {
      WriteTimedDataCsvReport(*view, report_path);
      co_return;
    }
    co_await Delay(executor, 100ms);
  }

  // Timed out with no samples: still emit whatever the view holds (a
  // header-only CSV) so the failing report is inspectable.
  if (auto* view = FindTimedDataView(app))
    WriteTimedDataCsvReport(*view, report_path);
  else
    WriteEmptyReport(report_path);
}

}  // namespace

Awaitable<void> RunE2eObjectViewValuesCheck(ClientApplication& app,
                                            AnyExecutor executor) {
  auto report_path = GetE2eObjectViewValuesReportPath();
  if (report_path.empty())
    co_return;

  co_await RunE2eObjectViewValuesCheck(
      ObjectViewValuesCheckContext{
          .executor = executor,
          .get_first_value_text = [&app]() -> std::optional<std::u16string> {
            if (auto* object_tree_view = FindObjectTreeView(app))
              return object_tree_view->GetFirstValueTextForTesting();
            return std::nullopt;
          },
      },
      std::move(report_path));
}

Awaitable<void> RunE2eObjectViewValuesCheck(
    ObjectViewValuesCheckContext context,
    std::filesystem::path report_path) {
  if (report_path.empty())
    co_return;

  co_await RunObjectViewValuesCheckAsync(std::move(context),
                                         std::move(report_path));
}

Awaitable<void> RunE2eOperatorUseCaseSmoke(ClientApplication& app) {
  auto report_path = GetE2eOperatorUseCasesReportPath();
  if (report_path.empty())
    co_return;

  auto executor = MakeAnyExecutor(std::make_shared<MessageLoopQt>());
  co_await RunE2eOperatorUseCaseSmoke(
      OperatorUseCaseSmokeContext{
          .executor = executor,
          .open_window =
              [&app, executor](std::string_view window_type) {
                return OpenOperatorWindowAsync(app, executor,
                                               std::string{window_type});
              },
          .is_window_registered =
              [](std::string_view window_type) {
                return FindWindowInfoByName(window_type) != nullptr;
              },
          .has_selection_command =
              [&app](unsigned command_id) {
                return app.HasSelectionCommandForTesting(command_id);
              },
          .has_global_command =
              [&app](unsigned command_id) {
                return app.HasGlobalCommandForTesting(command_id);
              },
          .has_main_window_command =
              [&app](unsigned command_id) {
                auto* main_window = GetFirstMainWindow(app);
                return main_window &&
                       main_window->commands().GetCommandHandler(command_id);
              },
          .has_context_menu_commands =
              [&app, executor](std::string_view window_type,
                               const std::vector<unsigned>& command_ids) {
                return CheckContextMenuCommandsAsync(
                    app, executor, std::string{window_type}, command_ids);
              },
          .is_window_printable =
              [](std::string_view window_type) {
                const auto* window_info = FindWindowInfoByName(window_type);
                return window_info && window_info->printable();
              },
      },
      std::move(report_path), MakeOperatorUseCaseSmokeChecks());
}

Awaitable<void> RunE2eOperatorUseCaseSmoke(
    OperatorUseCaseSmokeContext context,
    std::filesystem::path report_path,
    std::vector<OperatorUseCaseSmokeCheck> checks) {
  if (report_path.empty())
    co_return;

  co_await RunE2eOperatorUseCaseSmokeAsync(
      std::move(context), std::move(report_path), std::move(checks));
}

Awaitable<void> RunE2eObjectTreeLabelsCheck(ClientApplication& app,
                                            AnyExecutor executor) {
  auto report_path = GetE2eObjectTreeLabelsReportPath();
  if (report_path.empty())
    co_return;

  co_await RunE2eObjectTreeLabelsCheck(
      ObjectTreeLabelsCheckContext{
          .executor = executor,
          .get_expanded_labels =
              [&app] {
                if (auto* object_tree_view = FindObjectTreeView(app))
                  return object_tree_view->GetExpandedLabelPathForTesting(3);
                return std::vector<std::u16string>{};
              },
      },
      std::move(report_path));
}

Awaitable<void> RunE2eObjectTreeLabelsCheck(
    ObjectTreeLabelsCheckContext context,
    std::filesystem::path report_path) {
  if (report_path.empty())
    co_return;

  co_await RunObjectTreeLabelsCheckAsync(std::move(context),
                                         std::move(report_path));
}

Awaitable<void> RunE2eHardwareTreeDevicesCheck(ClientApplication& app,
                                               AnyExecutor executor) {
  auto report_path = GetE2eHardwareTreeDevicesReportPath();
  if (report_path.empty())
    co_return;

  auto check = std::make_shared<HardwareTreeDevicesCheck>(
      app, std::move(executor), std::move(report_path));
  co_await check->RunAsync();
}

Awaitable<void> RunE2eHistoricalTimedDataCheck(ClientApplication& app,
                                               AnyExecutor executor) {
  auto report_path = GetE2eHistoricalTimedDataReportPath();
  if (report_path.empty())
    co_return;

  co_await RunHistoricalTimedDataCheckAsync(app, executor,
                                            std::move(report_path));
}

Awaitable<void> RunE2eProfileSaveCheck(ClientApplication& app) {
  auto report_path = GetE2eProfileSaveReportPath();
  if (report_path.empty())
    co_return;

  Page page;
  page.title = u"E2E Server Profile Page";
  auto& added_page = app.profile().AddPage(page);

  auto target_user_id = NodeIdFromScadaString(GetE2eProfileSaveUserId());
  auto status = co_await app.SaveProfileToServer(std::move(target_user_id));
  WriteProfileSaveReport(report_path, scada::IsGood(status.code()), status,
                         added_page.id, added_page.title);
}

}  // namespace client
