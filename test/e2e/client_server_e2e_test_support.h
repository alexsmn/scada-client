#pragma once

#include "base/win/scoped_process_information.h"

#include <gtest/gtest.h>
#include <Windows.h>

#include <chrono>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace client::test {

enum class E2eProtocol {
  Remote,
  OpcUa,
};

std::string_view ToString(E2eProtocol protocol);

class JobObject;

struct ChildProcess {
  base::win::ScopedProcessInformation process_info;

  bool IsRunning() const;
  std::optional<DWORD> ExitCode() const;
};

class TempWorkspace {
 public:
  TempWorkspace();
  ~TempWorkspace();

  const std::filesystem::path& path() const { return path_; }

  void Preserve() { preserve_ = true; }

 private:
  std::filesystem::path path_;
  bool preserve_ = false;
};

std::string ReadFileOrEmpty(const std::filesystem::path& path);
bool ContainsInDirectory(const std::filesystem::path& dir,
                         std::string_view needle);
std::optional<int> FindLoggedObjectTreeChildCount(
    const std::filesystem::path& dir);

extern const std::chrono::seconds kPostConnectStabilityTimeout;
extern const std::string_view kStartupCompletedLog;

class ClientServerE2eTest : public ::testing::TestWithParam<E2eProtocol> {
 protected:
  ClientServerE2eTest();
  ~ClientServerE2eTest() override;

  void SetUp() override;
  void TearDown() override;

  void PrepareWorkspace();
  void WriteClientSettings(std::string_view password);
  void StartServer();
  void StartClient(std::vector<std::wstring> extra_args = {});

  std::string WaitForStatus();
  bool WaitForStartupOrStatus();
  bool WaitForObjectTreeReady();
  std::string WaitForOperatorUseCasesReport();

  std::string DescribeProcessExit(const ChildProcess& process,
                                  std::string_view name) const;
  void ExpectProcessesRemainRunningFor(std::chrono::milliseconds timeout,
                                       std::string_view context);
  void ExpectServerRemainsRunningFor(std::chrono::milliseconds timeout,
                                     std::string_view context);
  void ExpectServerAuthLog();

  TempWorkspace workspace_;
  std::unique_ptr<JobObject> job_;
  int remote_port_ = 0;
  int opcua_port_ = 0;

  std::filesystem::path status_file_;
  std::filesystem::path operator_use_cases_file_;
  std::filesystem::path settings_file_;
  std::filesystem::path server_log_dir_;
  std::filesystem::path client_log_dir_;

  ChildProcess server_;
  ChildProcess client_;

 private:
  int GetProtocolPort() const;
};

}  // namespace client::test
