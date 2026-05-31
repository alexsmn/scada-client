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

struct OperatorUseCaseSmokeCheck {
  std::string_view id;
  std::string_view description;
  std::vector<std::string_view> open_window_types;
  std::vector<std::string_view> registered_window_types;
  std::vector<unsigned> registered_selection_commands;
  std::vector<unsigned> registered_global_commands;
  std::vector<unsigned> main_window_commands;
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
  std::function<bool(std::string_view)> is_window_printable;
};

struct ObjectViewValuesCheckContext {
  AnyExecutor executor;
  std::function<std::optional<std::u16string>()> get_first_value_text;
  std::chrono::milliseconds timeout{30000};
  std::chrono::milliseconds poll_interval{100};
};

struct ObjectTreeLabelsCheckContext {
  AnyExecutor executor;
  std::function<std::vector<std::u16string>()> get_expanded_labels;
  std::chrono::milliseconds timeout{30000};
  std::chrono::milliseconds poll_interval{100};
};

Awaitable<void> RunE2eObjectViewValuesCheck(ClientApplication& app,
                                            AnyExecutor executor);
Awaitable<void> RunE2eObjectViewValuesCheck(ObjectViewValuesCheckContext context,
                                            std::filesystem::path report_path);
Awaitable<void> RunE2eOperatorUseCaseSmoke(ClientApplication& app);
Awaitable<void> RunE2eOperatorUseCaseSmoke(
    OperatorUseCaseSmokeContext context,
    std::filesystem::path report_path,
    std::vector<OperatorUseCaseSmokeCheck> checks);
Awaitable<void> RunE2eObjectTreeLabelsCheck(ClientApplication& app,
                                            AnyExecutor executor);
Awaitable<void> RunE2eObjectTreeLabelsCheck(ObjectTreeLabelsCheckContext context,
                                            std::filesystem::path report_path);
Awaitable<void> RunE2eHardwareTreeDevicesCheck(ClientApplication& app,
                                               AnyExecutor executor);

}  // namespace client
