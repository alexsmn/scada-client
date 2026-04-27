#include "app/qt/e2e_test_support.h"

#include "app/client_application.h"
#include "aui/qt/message_loop_qt.h"
#include "base/awaitable.h"
#include "base/awaitable_promise.h"
#include "base/callback_awaitable.h"
#include "base/e2e_test_hooks.h"
#include "base/executor_timer.h"
#include "base/executor_util.h"
#include "base/utf_convert.h"
#include "configuration/devices/hardware_tree_view.h"
#include "configuration/objects/object_tree_view.h"
#include "controller/command_handler.h"
#include "controller/window_info.h"
#include "main_window/main_window.h"
#include "main_window/main_window_manager.h"
#include "model/namespaces.h"
#include "model/node_id_util.h"
#include "main_window/opened_view.h"
#include "profile/window_definition.h"
#include "resources/common_resources.h"
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

void WriteOperatorUseCaseSmokeReport(
    const std::filesystem::path& path,
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
    output << "device[" << i << "].label="
           << UtfConvert<char>(devices[i].label) << "\n";
    output << "device[" << i << "].active="
           << (devices[i].active ? "true" : "false") << "\n";
    output << "device[" << i << "].state=" << devices[i].state << "\n";
  }
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

Awaitable<void> Delay(std::shared_ptr<Executor> executor,
                      std::chrono::milliseconds delay) {
  co_await CallbackToAwaitable<>(
      executor, [executor, delay](auto done) mutable {
        PostDelayedTask(NetExecutorAdapter{executor}, delay,
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
  do {
    labels = context.get_expanded_labels();
    if (IsObjectTreeLabelsReady(labels)) {
      WriteObjectTreeLabelsReport(report_path, true, labels,
                                  "expanded first rendered path");
      co_return;
    }

    if (std::chrono::steady_clock::now() >= deadline)
      break;

    co_await Delay(context.executor, context.poll_interval);
  } while (true);

  WriteObjectTreeLabelsReport(report_path, false, labels,
                              "timed out waiting for rendered labels");
  co_return;
}

class HardwareTreeDevicesCheck final
    : public std::enable_shared_from_this<HardwareTreeDevicesCheck> {
 public:
  HardwareTreeDevicesCheck(ClientApplication& app,
                           std::shared_ptr<Executor> executor,
                           std::filesystem::path report_path)
      : app_{app},
        executor_{std::move(executor)},
        report_path_{std::move(report_path)},
        deadline_{std::chrono::steady_clock::now() + 30s} {}

  Awaitable<void> RunAsync() {
    auto* main_window = GetFirstMainWindow(app_);
    const auto* window_info = FindWindowInfoByName("Subsystems");
    if (!main_window || !window_info) {
      WriteHardwareTreeDevicesReport(report_path_, false, {},
                                     "hardware tree window missing");
      co_return;
    }

    auto* opened_view = co_await AwaitPromise(
        NetExecutorAdapter{executor_},
        main_window->OpenView(WindowDefinition{*window_info},
                              /*activate=*/true));
    if (!opened_view) {
      WriteHardwareTreeDevicesReport(report_path_, false, {},
                                     "hardware tree failed to open");
      co_return;
    }

    StartProtocolActivity();
    co_await PollAsync();
  }

 private:
  static bool HasActiveDevice(
      const std::vector<HardwareTreeDeviceForTesting>& devices,
      std::string_view protocol) {
    return std::ranges::any_of(devices, [&](const auto& device) {
      return device.protocol == protocol && device.active;
    });
  }

  static bool IsReady(
      const std::vector<HardwareTreeDeviceForTesting>& devices) {
    return HasActiveDevice(devices, "MODBUS") &&
           HasActiveDevice(devices, "IEC60870") &&
           HasActiveDevice(devices, "IEC61850");
  }

  void StartProtocolActivity() {
    activation_specs_.clear();

    activation_specs_.emplace_back(
        app_.timed_data_service(),
        MakeNestedNodeId(scada::NodeId{2, NamespaceIndexes::MODBUS_DEVICES},
                         "BOOL:1"));
    activation_specs_.emplace_back(
        app_.timed_data_service(),
        MakeNestedNodeId(scada::NodeId{2, NamespaceIndexes::IEC60870_DEVICE},
                         "111"));
  }

  Awaitable<void> PollAsync() {
    while (std::chrono::steady_clock::now() < deadline_) {
      if (auto* hardware_tree_view = FindHardwareTreeView(app_)) {
        auto devices = hardware_tree_view->GetExpandedDevicesForTesting();
        if (IsReady(devices)) {
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
  std::shared_ptr<Executor> executor_;
  const std::filesystem::path report_path_;
  const std::chrono::steady_clock::time_point deadline_;
  std::vector<TimedDataSpec> activation_specs_;
};

Awaitable<OperatorUseCaseSmokeResult> OpenOperatorWindowAsync(
    ClientApplication& app,
    std::shared_ptr<Executor> executor,
    std::string window_type) {
  auto* main_window = GetFirstMainWindow(app);
  if (!main_window) {
    co_return OperatorUseCaseSmokeResult{.ok = false,
                                         .detail = "main window missing"};
  }

  if (const auto* window_info = FindWindowInfoByName(window_type)) {
    auto* opened_view = co_await AwaitPromise(
        NetExecutorAdapter{executor},
        main_window->OpenView(WindowDefinition{*window_info}, /*activate=*/true));
    co_return OperatorUseCaseSmokeResult{
        .ok = opened_view != nullptr,
        .detail = opened_view ? "opened" : "open returned null"};
  } else {
    co_return OperatorUseCaseSmokeResult{
        .ok = false, .detail = "window type not registered"};
  }
}

std::vector<OperatorUseCaseSmokeCheck> MakeOperatorUseCaseSmokeChecks() {
  return {
      {"UC-1", "monitor live values", {"Log"}},
      {"UC-2", "visualise time-series on a graph", {"Graph"}},
      {"UC-3", "view tables summaries and sheets", {"Table", "Summ", "CusTable"}},
      {"UC-4", "acknowledge events and alarms", {"Event"}},
      {"UC-5", "browse event journals", {"EventJournal"}},
      {"UC-6", "watch a custom spreadsheet", {"CusTable"}},
      {"UC-7", "issue control commands", {}, {}, {ID_WRITE, ID_WRITE_MANUAL}},
      {"UC-8", "manage favourites and portfolios", {"Favorites", "Portfolio"}},
      {"UC-9",
       "print or export the active view",
       {},
       {},
       {},
       {},
       {},
       {"Table", "EventJournal"}},
      {"UC-10", "browse server files", {"FileSystemView"}},
      {"UC-11", "view Modus and Vidicon schematics", {}, {"Modus", "VidiconDisplay"}},
      {"UC-12",
       "edit device parameters limits and aliases",
       {},
       {"NewProps", "Params"},
       {ID_EDIT_LIMITS}},
      {"UC-13",
       "bulk-create data items",
       {},
       {"TableEditor"}},
      {"UC-14",
       "export or import configuration",
       {},
       {},
       {},
       {ID_EXPORT_CONFIGURATION_TO_EXCEL, ID_IMPORT_CONFIGURATION_FROM_EXCEL}},
      {"UC-15",
       "inspect protocol traffic",
       {},
       {},
       {ID_DUMP_DEBUG_INFO}},
      {"UC-16",
       "save window layouts and profiles",
       {},
       {},
       {},
       {ID_PAGE_NEW, ID_PAGE_RENAME, ID_PAGE_DELETE}},
      {"UC-17",
       "authenticate against a back-end",
       {},
       {},
       {},
       {},
       {ID_LOGOFF}},
      {"UC-18",
       "manage users and passwords",
       {},
       {"Users"},
       {ID_CHANGE_PASSWORD},
       {},
       {ID_USERS_VIEW}},
      {"UC-19",
       "configure transmission rules",
       {},
       {"Transmission"}},
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
      auto result = co_await AwaitPromise(NetExecutorAdapter{executor},
                                          context.open_window(window_type));
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
      detail += available ? " main-window-command "
                          : " missing-main-window-command ";
      detail += std::to_string(command_id);
    }

    for (std::string_view window_type : check.printable_window_types) {
      bool printable = context.is_window_printable(window_type);
      ok = ok && printable;
      detail += printable ? " printable " : " not-printable ";
      detail += window_type;
    }

    AddSmokeResult(state, check.id, ok, std::move(detail));
  }

  WriteOperatorUseCaseSmokeReport(report_path, *state);
  co_return;
}

}  // namespace

promise<> RunE2eObjectViewValuesCheck(ClientApplication& app,
                                      std::shared_ptr<Executor> executor) {
  auto report_path = GetE2eObjectViewValuesReportPath();
  if (report_path.empty())
    return make_resolved_promise();

  return RunE2eObjectViewValuesCheck(ObjectViewValuesCheckContext{
      .executor = executor,
      .get_first_value_text =
          [&app]() -> std::optional<std::u16string> {
            if (auto* object_tree_view = FindObjectTreeView(app))
              return object_tree_view->GetFirstValueTextForTesting();
            return std::nullopt;
          },
  }, std::move(report_path));
}

promise<> RunE2eObjectViewValuesCheck(ObjectViewValuesCheckContext context,
                                      std::filesystem::path report_path) {
  if (report_path.empty())
    return make_resolved_promise();

  auto executor = context.executor;
  return ToPromise(NetExecutorAdapter{executor},
                   RunObjectViewValuesCheckAsync(std::move(context),
                                                 std::move(report_path)));
}

promise<> RunE2eOperatorUseCaseSmoke(ClientApplication& app) {
  auto report_path = GetE2eOperatorUseCasesReportPath();
  if (report_path.empty())
    return make_resolved_promise();

  auto executor = std::make_shared<MessageLoopQt>();
  return RunE2eOperatorUseCaseSmoke(OperatorUseCaseSmokeContext{
      .executor = executor,
      .open_window =
          [&app, executor](std::string_view window_type) {
            return ToPromise(NetExecutorAdapter{executor},
                             OpenOperatorWindowAsync(
                                 app, executor, std::string{window_type}));
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
      .is_window_printable =
          [](std::string_view window_type) {
            const auto* window_info = FindWindowInfoByName(window_type);
            return window_info && window_info->printable();
          },
  }, std::move(report_path), MakeOperatorUseCaseSmokeChecks());
}

promise<> RunE2eOperatorUseCaseSmoke(
    OperatorUseCaseSmokeContext context,
    std::filesystem::path report_path,
    std::vector<OperatorUseCaseSmokeCheck> checks) {
  if (report_path.empty())
    return make_resolved_promise();

  auto executor = context.executor;
  return ToPromise(NetExecutorAdapter{executor},
                   RunE2eOperatorUseCaseSmokeAsync(std::move(context),
                                                   std::move(report_path),
                                                   std::move(checks)));
}

promise<> RunE2eObjectTreeLabelsCheck(ClientApplication& app,
                                      std::shared_ptr<Executor> executor) {
  auto report_path = GetE2eObjectTreeLabelsReportPath();
  if (report_path.empty())
    return make_resolved_promise();

  return RunE2eObjectTreeLabelsCheck(ObjectTreeLabelsCheckContext{
      .executor = executor,
      .get_expanded_labels =
          [&app] {
            if (auto* object_tree_view = FindObjectTreeView(app))
              return object_tree_view->GetExpandedLabelPathForTesting(3);
            return std::vector<std::u16string>{};
          },
  }, std::move(report_path));
}

promise<> RunE2eObjectTreeLabelsCheck(ObjectTreeLabelsCheckContext context,
                                      std::filesystem::path report_path) {
  if (report_path.empty())
    return make_resolved_promise();

  auto executor = context.executor;
  return ToPromise(NetExecutorAdapter{executor},
                   RunObjectTreeLabelsCheckAsync(std::move(context),
                                                 std::move(report_path)));
}

promise<> RunE2eHardwareTreeDevicesCheck(ClientApplication& app,
                                         std::shared_ptr<Executor> executor) {
  auto report_path = GetE2eHardwareTreeDevicesReportPath();
  if (report_path.empty())
    return make_resolved_promise();

  auto promise_executor = executor;
  auto check = std::make_shared<HardwareTreeDevicesCheck>(
      app, std::move(executor), std::move(report_path));
  promise<> result;
  CoSpawn(promise_executor, [check, result]() mutable -> Awaitable<void> {
    try {
      co_await check->RunAsync();
      result.resolve();
    } catch (...) {
      result.reject(std::current_exception());
    }
  });
  return result;
}

}  // namespace client
