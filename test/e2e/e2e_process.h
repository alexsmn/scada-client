#pragma once

#include "base/win/scoped_handle.h"
#include "base/win/scoped_process_information.h"

#include <Windows.h>

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace client::test {

class JobObject {
 public:
  JobObject();

  void Assign(HANDLE process);

 private:
  base::win::ScopedHandle handle_;
};

struct ChildProcess {
  base::win::ScopedProcessInformation process_info;

  bool IsRunning() const;
  std::optional<DWORD> ExitCode() const;
};

void ForceTerminate(base::win::ScopedProcessInformation& process_info);
void WaitForExit(base::win::ScopedProcessInformation& process_info,
                 DWORD timeout_ms = 5000);
void LaunchProcess(const std::filesystem::path& exe,
                   const std::vector<std::wstring>& args,
                   const std::filesystem::path& workdir,
                   JobObject& job,
                   ChildProcess& child);

}  // namespace client::test
