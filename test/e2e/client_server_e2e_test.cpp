#include "test/e2e/client_server_e2e_test_support.h"

#include <boost/json.hpp>

#include <chrono>
#include <sstream>
#include <string>
#include <string_view>

namespace client::test {
namespace {

bool HasActiveHardwareTreeDevice(std::string_view report,
                                 std::string_view protocol) {
  std::istringstream stream{std::string{report}};
  std::string line;
  bool in_matching_device = false;
  while (std::getline(stream, line)) {
    if (line.starts_with("device[") &&
        line.find(".protocol=") != std::string::npos) {
      in_matching_device =
          line.find(std::string{".protocol="} + std::string{protocol}) !=
          std::string::npos;
      continue;
    }
    if (in_matching_device && line.find(".active=true") != std::string::npos)
      return true;
  }
  return false;
}

bool ProfileJsonContainsPageTitle(std::string_view profile_json,
                                  std::string_view page_title) {
  auto value = boost::json::parse(profile_json);
  auto* pages = value.as_object().if_contains("pages");
  if (!pages || !pages->is_array())
    return false;

  for (const auto& page : pages->as_array()) {
    if (!page.is_object())
      continue;
    auto* title = page.as_object().if_contains("title");
    if (title && title->is_string() && title->as_string() == page_title)
      return true;
  }
  return false;
}

TEST_P(ClientServerE2eTest, Connect_Success) {
  WriteClientSettings(/*password=*/"");
  StartServer();
  StartClient();

  ASSERT_TRUE(WaitForStartupOrStatus())
      << "Timed out waiting for client startup/status signal";
  const auto status = ReadFileOrEmpty(status_file_);
  EXPECT_TRUE(ContainsInDirectory(client_log_dir_, kStartupCompletedLog))
      << "Client did not log startup completion; status: " << status;
  EXPECT_TRUE(status.empty() || status == "success")
      << "Unexpected client status while waiting for startup: " << status;
  EXPECT_TRUE(client_.IsRunning()) << "Client exited unexpectedly after login";

  ExpectServerAuthLog();
  ExpectProcessesRemainRunningFor(
      std::chrono::duration_cast<std::chrono::milliseconds>(
          kPostConnectStabilityTimeout),
      "waiting for the post-login session to remain stable");
}

TEST_P(ClientServerE2eTest, Connect_Success_LoadsObjectTree) {
  WriteClientSettings(/*password=*/"");
  StartServer();
  StartClient(
      {"--test-object-view-values-file=" + object_view_values_file_.string()});

  ASSERT_TRUE(WaitForStartupOrStatus())
      << "Timed out waiting for client startup/status signal";
  const auto status = ReadFileOrEmpty(status_file_);
  ASSERT_TRUE(ContainsInDirectory(client_log_dir_, kStartupCompletedLog))
      << "Client did not log startup completion; status: " << status;
  ASSERT_TRUE(status.empty() || status == "success")
      << "Unexpected client status while waiting for startup: " << status;
  ASSERT_TRUE(client_.IsRunning()) << "Client exited unexpectedly after login";

  ASSERT_TRUE(WaitForObjectTreeReady())
      << "Timed out waiting for object tree ready signal; status: "
      << ReadFileOrEmpty(status_file_);
  const auto child_count = FindLoggedObjectTreeChildCount(client_log_dir_);
  ASSERT_TRUE(child_count)
      << "Client reported startup ready but client logs never showed a "
         "completed object tree fetch";
  EXPECT_GT(*child_count, 0)
      << "Client loaded the object tree root but did not materialize any "
         "first-level children";
  const auto object_values_report = WaitForObjectViewValuesReport();
  EXPECT_NE(object_values_report.find("object-view-values: ok"),
            std::string::npos)
      << object_values_report;

  ExpectServerAuthLog();
  ExpectProcessesRemainRunningFor(
      std::chrono::duration_cast<std::chrono::milliseconds>(
          kPostConnectStabilityTimeout),
      "waiting for the object tree to remain available after login");
}

TEST_P(ClientServerE2eTest, Connect_Success_ExpandsObjectTreeLabels) {
  WriteClientSettings(/*password=*/"");
  StartServer();
  StartClient(
      {"--test-object-tree-labels-file=" + object_tree_labels_file_.string()});

  ASSERT_TRUE(WaitForStartupOrStatus())
      << "Timed out waiting for client startup/status signal";
  const auto status = ReadFileOrEmpty(status_file_);
  ASSERT_TRUE(ContainsInDirectory(client_log_dir_, kStartupCompletedLog))
      << "Client did not log startup completion; status: " << status;
  ASSERT_TRUE(status.empty() || status == "success")
      << "Unexpected client status while waiting for startup: " << status;
  ASSERT_TRUE(client_.IsRunning()) << "Client exited unexpectedly after login";

  const auto report = WaitForObjectTreeLabelsReport();
  ASSERT_NE(report.find("object-tree-labels: ok"), std::string::npos) << report;
  for (std::string_view expected_label :
       {"label[0]=Все объекты", "label[1]=Отрадная 110 КВ", "label[2]=ТС",
        "label[3]=МВ-35 У"}) {
    EXPECT_NE(report.find(expected_label), std::string::npos)
        << "Missing expected object tree label " << expected_label
        << " in report:\n"
        << report;
  }

  ExpectServerAuthLog();
  ExpectProcessesRemainRunningFor(
      std::chrono::duration_cast<std::chrono::milliseconds>(
          kPostConnectStabilityTimeout),
      "waiting for expanded object tree labels to remain available");
}

TEST_P(ClientServerE2eTest, Connect_Success_ExpandsHardwareTreeDevices) {
  WriteClientSettings(/*password=*/"");
  StartServer();
  StartClient({"--test-hardware-tree-devices-file=" +
               hardware_tree_devices_file_.string()});

  ASSERT_TRUE(WaitForStartupOrStatus())
      << "Timed out waiting for client startup/status signal";
  const auto status = ReadFileOrEmpty(status_file_);
  ASSERT_TRUE(ContainsInDirectory(client_log_dir_, kStartupCompletedLog))
      << "Client did not log startup completion; status: " << status;
  ASSERT_TRUE(status.empty() || status == "success")
      << "Unexpected client status while waiting for startup: " << status;
  ASSERT_TRUE(client_.IsRunning()) << "Client exited unexpectedly after login";

  const auto report = WaitForHardwareTreeDevicesReport();
  ASSERT_NE(report.find("hardware-tree-devices: ok"), std::string::npos)
      << report;
  for (std::string_view protocol : {"MODBUS", "IEC60870", "IEC61850"}) {
    EXPECT_TRUE(HasActiveHardwareTreeDevice(report, protocol))
        << "Hardware tree device for " << protocol
        << " was not active in report:\n"
        << report;
  }

  ExpectServerAuthLog();
  ExpectProcessesRemainRunningFor(
      std::chrono::duration_cast<std::chrono::milliseconds>(
          kPostConnectStabilityTimeout),
      "waiting for expanded hardware tree devices to remain available");
}

TEST_P(ClientServerE2eTest, OperatorUseCases_OpenRegisteredSurfaces) {
  WriteClientSettings(/*password=*/"");
  StartServer();
  StartClient(
      {"--test-operator-use-cases-file=" + operator_use_cases_file_.string(),
       "--debug"});

  ASSERT_TRUE(WaitForStartupOrStatus())
      << "Timed out waiting for client startup/status signal";
  const auto status = ReadFileOrEmpty(status_file_);
  ASSERT_TRUE(ContainsInDirectory(client_log_dir_, kStartupCompletedLog))
      << "Client did not log startup completion; status: " << status;
  ASSERT_TRUE(status.empty() || status == "success")
      << "Unexpected client status while waiting for startup: " << status;
  ASSERT_TRUE(client_.IsRunning()) << "Client exited unexpectedly after login";

  const auto report = WaitForOperatorUseCasesReport();
  ASSERT_NE(report.find("operator-use-cases: ok"), std::string::npos) << report;

  for (std::string_view use_case :
       {"UC-1", "UC-2", "UC-3", "UC-4", "UC-5", "UC-6", "UC-7", "UC-8", "UC-9",
        "UC-10", "UC-11", "UC-12", "UC-13", "UC-14", "UC-15", "UC-16", "UC-17",
        "UC-18", "UC-19"}) {
    EXPECT_NE(report.find(std::string{use_case} + " ok"), std::string::npos)
        << "Missing successful operator use-case coverage for " << use_case
        << " in report:\n"
        << report;
  }

  ExpectServerAuthLog();
#if !defined(__APPLE__)
  ExpectProcessesRemainRunningFor(
      std::chrono::duration_cast<std::chrono::milliseconds>(
          kPostConnectStabilityTimeout),
      "waiting for operator use-case surfaces to remain stable");
#endif
}

TEST_P(ClientServerE2eTest, ProfileSave_PersistsPagesOnServer) {
  constexpr int kGuestUserId = 12;
  constexpr std::string_view kSavedPageTitle = "E2E Server Profile Page";

  WriteClientSettings(/*password=*/"", /*user=*/"guest");
  StartServer();
  StartClient({"--test-profile-save-file=" + profile_save_file_.string(),
               "--test-profile-save-user-id=USER.12"});

  ASSERT_TRUE(WaitForStartupOrStatus())
      << "Timed out waiting for client startup/status signal";
  const auto status = ReadFileOrEmpty(status_file_);
  ASSERT_TRUE(ContainsInDirectory(client_log_dir_, kStartupCompletedLog))
      << "Client did not log startup completion; status: " << status;
  ASSERT_TRUE(status.empty() || status == "success")
      << "Unexpected client status while waiting for startup: " << status;
  ASSERT_TRUE(client_.IsRunning()) << "Client exited unexpectedly after login";

  const auto report = WaitForProfileSaveReport();
  ASSERT_NE(report.find("profile-save: ok"), std::string::npos) << report;
  ASSERT_NE(report.find("page-title=E2E Server Profile Page"),
            std::string::npos)
      << report;

  const auto profile_json = ReadUserProfileJsonFromServerDatabase(kGuestUserId);
  EXPECT_TRUE(ProfileJsonContainsPageTitle(profile_json, kSavedPageTitle))
      << profile_json;
  EXPECT_EQ(ReadUserProfileRevisionFromServerDatabase(kGuestUserId), "1");

  ExpectServerAuthLog();
  ExpectProcessesRemainRunningFor(
      std::chrono::duration_cast<std::chrono::milliseconds>(
          kPostConnectStabilityTimeout),
      "waiting after profile pages were saved to the server");
}

TEST_P(ClientServerE2eTest, Connect_BadPassword) {
  WriteClientSettings("wrong-password");
  StartServer();
  StartClient();

  auto status = WaitForStatus();
  EXPECT_NE(status.find("failure: Bad_WrongLoginCredentials"),
            std::string::npos)
      << "Unexpected client status: " << status;
  EXPECT_FALSE(ContainsInDirectory(server_log_dir_, "Authorization succeeded"))
      << "Server should not record successful authorization";
  ExpectServerRemainsRunningFor(
      std::chrono::duration_cast<std::chrono::milliseconds>(
          kPostConnectStabilityTimeout),
      "waiting for the server to remain stable after rejecting bad "
      "credentials");
}

TEST_P(ClientServerE2eTest, Connect_Success_WithDiscoveryAutoSecurity) {
  if (Protocol() != E2eProtocol::OpcUa)
    GTEST_SKIP() << "Endpoint discovery/security applies to the OPC UA backend";

  WriteClientSettings(/*password=*/"", /*user=*/"root",
                      /*security_mode=*/"Auto");
  StartServer();
  StartClient();

  ASSERT_TRUE(WaitForStartupOrStatus())
      << "Timed out waiting for client startup/status signal";
  const auto status = ReadFileOrEmpty(status_file_);
  EXPECT_TRUE(ContainsInDirectory(client_log_dir_, kStartupCompletedLog))
      << "Client did not log startup completion; status: " << status;
  EXPECT_TRUE(status.empty() || status == "success")
      << "Unexpected client status while waiting for startup: " << status;
  EXPECT_TRUE(client_.IsRunning()) << "Client exited unexpectedly after login";

  // "Auto" security can only connect if GetEndpoints discovery ran and selected
  // the server's (None) endpoint, so a successful login proves that path. The
  // post-activation NamespaceArray read is part of the same new flow.
  EXPECT_TRUE(ContainsInDirectory(client_log_dir_, "NamespaceArray"))
      << "Client did not read the server NamespaceArray after connecting";

  ExpectServerAuthLog();
  ExpectProcessesRemainRunningFor(
      std::chrono::duration_cast<std::chrono::milliseconds>(
          kPostConnectStabilityTimeout),
      "waiting for the discovery-based OPC UA session to remain stable");
}

TEST_P(ClientServerE2eTest,
       Connect_SignAndEncryptRejectedWhenServerOffersNone) {
  if (Protocol() != E2eProtocol::OpcUa)
    GTEST_SKIP() << "Endpoint security applies to the OPC UA backend";

  // The in-repo server advertises only a SecurityPolicy=None endpoint, so a
  // client that requires SignAndEncrypt must fail endpoint selection during
  // discovery and never activate a session.
  WriteClientSettings(/*password=*/"", /*user=*/"root",
                      /*security_mode=*/"SignAndEncrypt");
  StartServer();
  StartClient();

  const auto status = WaitForStatus();
  EXPECT_NE(status.find("failure"), std::string::npos)
      << "Expected the client to report a connection failure; status: "
      << status;
  EXPECT_FALSE(ContainsInDirectory(server_log_dir_, "OPC UA session activated"))
      << "Server should not activate a session when security selection fails";
  ExpectServerRemainsRunningFor(
      std::chrono::duration_cast<std::chrono::milliseconds>(
          kPostConnectStabilityTimeout),
      "waiting for the server to remain stable after rejecting an unsupported "
      "security mode");
}

INSTANTIATE_TEST_SUITE_P(
    Protocols,
    ClientServerE2eTest,
    ::testing::Values(
        E2eParam{E2eProtocol::Remote, ServerTopology::Monolith},
        E2eParam{E2eProtocol::OpcUa, ServerTopology::Monolith},
        E2eParam{E2eProtocol::Remote, ServerTopology::MultiProcess},
        E2eParam{E2eProtocol::OpcUa, ServerTopology::MultiProcess}),
    [](const ::testing::TestParamInfo<E2eParam>& info) {
      return E2eParamName(info.param);
    });

}  // namespace
}  // namespace client::test
