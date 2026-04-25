#include "test/e2e/e2e_process.h"

#include <stdexcept>

namespace client::test {
namespace {

std::wstring QuotePathArg(const std::filesystem::path& arg) {
  return L"\"" + arg.wstring() + L"\"";
}

std::wstring QuoteStringArg(std::wstring_view arg) {
  return L"\"" + std::wstring{arg} + L"\"";
}

}  // namespace

JobObject::JobObject() {
  handle_.Set(CreateJobObjectW(nullptr, nullptr));
  if (!handle_.IsValid())
    throw std::runtime_error{"CreateJobObjectW failed"};

  JOBOBJECT_EXTENDED_LIMIT_INFORMATION info{};
  info.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
  if (!SetInformationJobObject(handle_.Get(),
                               JobObjectExtendedLimitInformation,
                               &info,
                               sizeof(info))) {
    throw std::runtime_error{"SetInformationJobObject failed"};
  }
}

void JobObject::Assign(HANDLE process) {
  if (!AssignProcessToJobObject(handle_.Get(), process))
    throw std::runtime_error{"AssignProcessToJobObject failed"};
}

bool ChildProcess::IsRunning() const {
  if (!process_info.process_handle())
    return false;
  DWORD exit_code = 0;
  return GetExitCodeProcess(process_info.process_handle(), &exit_code) &&
         exit_code == STILL_ACTIVE;
}

std::optional<DWORD> ChildProcess::ExitCode() const {
  if (!process_info.process_handle())
    return std::nullopt;

  DWORD exit_code = 0;
  if (!GetExitCodeProcess(process_info.process_handle(), &exit_code))
    return std::nullopt;
  return exit_code;
}

void ForceTerminate(base::win::ScopedProcessInformation& process_info) {
  if (!process_info.process_handle())
    return;

  DWORD exit_code = 0;
  if (GetExitCodeProcess(process_info.process_handle(), &exit_code) &&
      exit_code == STILL_ACTIVE) {
    TerminateProcess(process_info.process_handle(), 1);
    WaitForSingleObject(process_info.process_handle(), 5000);
  }
}

void WaitForExit(base::win::ScopedProcessInformation& process_info,
                 DWORD timeout_ms) {
  if (!process_info.process_handle())
    return;
  WaitForSingleObject(process_info.process_handle(), timeout_ms);
}

void LaunchProcess(const std::filesystem::path& exe,
                   const std::vector<std::wstring>& args,
                   const std::filesystem::path& workdir,
                   JobObject& job,
                   ChildProcess& child) {
  std::wstring command_line = QuotePathArg(exe);
  for (const auto& arg : args) {
    command_line += L" ";
    command_line += QuoteStringArg(arg);
  }

  STARTUPINFOW startup_info{};
  startup_info.cb = sizeof(startup_info);

  std::vector<wchar_t> mutable_command(command_line.begin(), command_line.end());
  mutable_command.push_back(L'\0');

  if (!CreateProcessW(exe.c_str(),
                      mutable_command.data(),
                      nullptr,
                      nullptr,
                      FALSE,
                      0,
                      nullptr,
                      workdir.c_str(),
                      &startup_info,
                      child.process_info.Receive())) {
    throw std::runtime_error{"CreateProcessW failed for " + exe.string()};
  }

  job.Assign(child.process_info.process_handle());
}

}  // namespace client::test
