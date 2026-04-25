#pragma once

#include "test/e2e/e2e_file_helpers.h"
#include "test/e2e/e2e_process.h"

#include <gtest/gtest.h>

#include <chrono>
#include <filesystem>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace client::test {

class Iec61850TestServer;

enum class E2eProtocol {
  Remote,
  OpcUa,
};

std::string_view ToString(E2eProtocol protocol);

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
  std::string WaitForObjectViewValuesReport();
  std::string WaitForObjectTreeLabelsReport();
  std::string WaitForHardwareTreeDevicesReport();
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
  int iec61850_port_ = 0;
  std::unique_ptr<Iec61850TestServer> iec61850_server_;

  std::filesystem::path status_file_;
  std::filesystem::path object_view_values_file_;
  std::filesystem::path object_tree_labels_file_;
  std::filesystem::path hardware_tree_devices_file_;
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
