#pragma once

#include "base/promise.h"

#include <chrono>
#include <filesystem>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

class ClientApplication;
class Executor;

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
};

struct OperatorUseCaseSmokeResult {
  bool ok = true;
  std::string detail;
};

struct OperatorUseCaseSmokeContext {
  std::shared_ptr<Executor> executor;
  std::function<promise<OperatorUseCaseSmokeResult>(std::string_view)>
      open_window;
  std::function<bool(std::string_view)> is_window_registered;
  std::function<bool(unsigned)> has_selection_command;
  std::function<bool(unsigned)> has_global_command;
  std::function<bool(unsigned)> has_main_window_command;
  std::function<bool(std::string_view)> is_window_printable;
};

struct ObjectViewValuesCheckContext {
  std::shared_ptr<Executor> executor;
  std::function<std::optional<std::u16string>()> get_first_value_text;
  std::chrono::milliseconds timeout{30000};
  std::chrono::milliseconds poll_interval{100};
};

struct ObjectTreeLabelsCheckContext {
  std::shared_ptr<Executor> executor;
  std::function<std::vector<std::u16string>()> get_expanded_labels;
  std::chrono::milliseconds timeout{30000};
  std::chrono::milliseconds poll_interval{100};
};

promise<> RunE2eObjectViewValuesCheck(ClientApplication& app,
                                      std::shared_ptr<Executor> executor);
promise<> RunE2eObjectViewValuesCheck(ObjectViewValuesCheckContext context,
                                      std::filesystem::path report_path);
promise<> RunE2eOperatorUseCaseSmoke(ClientApplication& app);
promise<> RunE2eOperatorUseCaseSmoke(OperatorUseCaseSmokeContext context,
                                     std::filesystem::path report_path,
                                     std::vector<OperatorUseCaseSmokeCheck> checks);
promise<> RunE2eObjectTreeLabelsCheck(ClientApplication& app,
                                      std::shared_ptr<Executor> executor);
promise<> RunE2eObjectTreeLabelsCheck(ObjectTreeLabelsCheckContext context,
                                      std::filesystem::path report_path);
promise<> RunE2eHardwareTreeDevicesCheck(ClientApplication& app,
                                         std::shared_ptr<Executor> executor);

}  // namespace client
