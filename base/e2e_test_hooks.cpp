#include "base/e2e_test_hooks.h"

#include "base/file_settings_store.h"
#include "base/program_options.h"

#include <atomic>
#include <fstream>

namespace client {
namespace {

constexpr std::string_view kTestSettingsFileOption = "test-settings-file";
constexpr std::string_view kTestStatusFileOption = "test-status-file";

std::atomic_bool& GetStatusReported() {
  static std::atomic_bool status_reported = false;
  return status_reported;
}

std::filesystem::path GetOptionPath(std::string_view option_name) {
  auto value = GetOptionValue(option_name);
  return value.empty() ? std::filesystem::path{} : std::filesystem::path{value};
}

void WriteFile(const std::filesystem::path& path, std::string_view contents) {
  if (path.empty())
    return;

  std::error_code ec;
  if (path.has_parent_path())
    std::filesystem::create_directories(path.parent_path(), ec);

  std::ofstream output{path, std::ios::binary | std::ios::trunc};
  if (!output)
    return;
  output << contents;
}

}  // namespace

bool IsE2eTestMode() {
  return !GetOptionValue(kTestSettingsFileOption).empty();
}

std::shared_ptr<SettingsStore> CreateE2eSettingsStore() {
  auto path = GetOptionPath(kTestSettingsFileOption);
  if (path.empty())
    return {};
  return std::make_shared<FileSettingsStore>(std::move(path));
}

void ReportE2eStatus(std::string_view status) {
  WriteFile(GetOptionPath(kTestStatusFileOption), status);
  GetStatusReported().store(true);
}

void ReportE2eStatusIfUnset(std::string_view status) {
  bool expected = false;
  if (GetStatusReported().compare_exchange_strong(expected, true)) {
    WriteFile(GetOptionPath(kTestStatusFileOption), status);
  }
}

}  // namespace client
