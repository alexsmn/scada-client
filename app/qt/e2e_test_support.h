#pragma once

#include "base/any_executor.h"

#include "base/awaitable.h"

#include <chrono>
#include <filesystem>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

class ClientApplication;

namespace client {

struct ContextMenuSmokeCheck {
  std::string_view window_type;
  std::vector<unsigned> command_ids;
};

struct OperatorUseCaseSmokeCheck {
  std::string_view id;
  std::string_view description;
  std::vector<std::string_view> open_window_types;
  std::vector<std::string_view> registered_window_types;
  std::vector<unsigned> registered_selection_commands;
  std::vector<unsigned> registered_global_commands;
  std::vector<unsigned> main_window_commands;
  std::vector<ContextMenuSmokeCheck> context_menu_commands;
  std::vector<std::string_view> printable_window_types;
  bool optional_when_unavailable = false;
};

struct OperatorUseCaseSmokeResult {
  bool ok = true;
  std::string detail;
};

struct OperatorUseCaseSmokeContext {
  AnyExecutor executor;
  std::function<Awaitable<OperatorUseCaseSmokeResult>(std::string_view)>
      open_window;
  std::function<bool(std::string_view)> is_window_registered;
  std::function<bool(unsigned)> has_selection_command;
  std::function<bool(unsigned)> has_global_command;
  std::function<bool(unsigned)> has_main_window_command;
  std::function<Awaitable<OperatorUseCaseSmokeResult>(
      std::string_view,
      const std::vector<unsigned>&)>
      has_context_menu_commands;
  std::function<bool(std::string_view)> is_window_printable;
};

struct ObjectViewValuesCheckContext {
  AnyExecutor executor;
  std::function<std::optional<std::u16string>()> get_first_value_text;
  // Client-side capture budget. Under full-suite load many tier processes
  // saturate the CPU and browse + attribute/value delivery legitimately takes
  // longer than 30s, so the check must keep polling well past that before
  // giving up (the matching test-side wait is strictly larger — see
  // kObjectViewValuesTimeout).
  std::chrono::milliseconds timeout{60000};
  std::chrono::milliseconds poll_interval{100};
};

struct ObjectTreeLabelsCheckContext {
  AnyExecutor executor;
  std::function<std::vector<std::u16string>()> get_expanded_labels;
  // See ObjectViewValuesCheckContext::timeout — the same load-sensitivity
  // applies to DisplayName resolution along the object tree path.
  std::chrono::milliseconds timeout{60000};
  std::chrono::milliseconds poll_interval{100};
  // How long a candidate path must stay unchanged before it is accepted.
  // The capture walks the first child that reaches the requested depth while
  // FetchMore() is still in flight, so a transient subtree can briefly present
  // a complete-looking path before the intended one finishes loading. Requiring
  // the same path across this window means "the tree has settled" rather than
  // merely "four labels are non-empty".
  std::chrono::milliseconds settle_duration{2000};
};

Awaitable<void> RunE2eObjectViewValuesCheck(ClientApplication& app,
                                            AnyExecutor executor);
Awaitable<void> RunE2eObjectViewValuesCheck(
    ObjectViewValuesCheckContext context,
    std::filesystem::path report_path);
Awaitable<void> RunE2eOperatorUseCaseSmoke(ClientApplication& app);
Awaitable<void> RunE2eOperatorUseCaseSmoke(
    OperatorUseCaseSmokeContext context,
    std::filesystem::path report_path,
    std::vector<OperatorUseCaseSmokeCheck> checks);
Awaitable<void> RunE2eObjectTreeLabelsCheck(ClientApplication& app,
                                            AnyExecutor executor);
Awaitable<void> RunE2eObjectTreeLabelsCheck(
    ObjectTreeLabelsCheckContext context,
    std::filesystem::path report_path);
Awaitable<void> RunE2eHardwareTreeDevicesCheck(ClientApplication& app,
                                               AnyExecutor executor);
Awaitable<void> RunE2eHistoricalTimedDataCheck(ClientApplication& app,
                                               AnyExecutor executor);
Awaitable<void> RunE2eProfileSaveCheck(ClientApplication& app);

}  // namespace client
