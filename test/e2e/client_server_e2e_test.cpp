#include "test/e2e/client_server_e2e_test_support.h"

#include <chrono>
#include <string>

namespace client::test {
namespace {

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
      {L"--test-object-view-values-file=" +
       object_view_values_file_.wstring()});

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
      {L"--test-object-tree-labels-file=" +
       object_tree_labels_file_.wstring()});

  ASSERT_TRUE(WaitForStartupOrStatus())
      << "Timed out waiting for client startup/status signal";
  const auto status = ReadFileOrEmpty(status_file_);
  ASSERT_TRUE(ContainsInDirectory(client_log_dir_, kStartupCompletedLog))
      << "Client did not log startup completion; status: " << status;
  ASSERT_TRUE(status.empty() || status == "success")
      << "Unexpected client status while waiting for startup: " << status;
  ASSERT_TRUE(client_.IsRunning()) << "Client exited unexpectedly after login";

  const auto report = WaitForObjectTreeLabelsReport();
  ASSERT_NE(report.find("object-tree-labels: ok"), std::string::npos)
      << report;
  for (std::string_view expected_label :
       {"label[0]=Все объекты",
        "label[1]=Отрадная 110 КВ",
        "label[2]=ТС",
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

TEST_P(ClientServerE2eTest, OperatorUseCases_OpenRegisteredSurfaces) {
  WriteClientSettings(/*password=*/"");
  StartServer();
  StartClient(
      {L"--test-operator-use-cases-file=" +
           operator_use_cases_file_.wstring(),
       L"--debug"});

  ASSERT_TRUE(WaitForStartupOrStatus())
      << "Timed out waiting for client startup/status signal";
  const auto status = ReadFileOrEmpty(status_file_);
  ASSERT_TRUE(ContainsInDirectory(client_log_dir_, kStartupCompletedLog))
      << "Client did not log startup completion; status: " << status;
  ASSERT_TRUE(status.empty() || status == "success")
      << "Unexpected client status while waiting for startup: " << status;
  ASSERT_TRUE(client_.IsRunning()) << "Client exited unexpectedly after login";

  const auto report = WaitForOperatorUseCasesReport();
  ASSERT_NE(report.find("operator-use-cases: ok"), std::string::npos)
      << report;

  for (std::string_view use_case :
       {"UC-1", "UC-2", "UC-3", "UC-4", "UC-5", "UC-6",
        "UC-7", "UC-8", "UC-9", "UC-10", "UC-11", "UC-12",
        "UC-13", "UC-14", "UC-15", "UC-16", "UC-17", "UC-18",
        "UC-19"}) {
    EXPECT_NE(report.find(std::string{use_case} + " ok"), std::string::npos)
        << "Missing successful operator use-case coverage for " << use_case
        << " in report:\n"
        << report;
  }

  ExpectServerAuthLog();
  ExpectProcessesRemainRunningFor(
      std::chrono::duration_cast<std::chrono::milliseconds>(
          kPostConnectStabilityTimeout),
      "waiting for operator use-case surfaces to remain stable");
}

TEST_P(ClientServerE2eTest, Connect_BadPassword) {
  WriteClientSettings("wrong-password");
  StartServer();
  StartClient();

  auto status = WaitForStatus();
  EXPECT_NE(status.find("failure: Bad_WrongLoginCredentials"), std::string::npos)
      << "Unexpected client status: " << status;
  EXPECT_FALSE(ContainsInDirectory(server_log_dir_, "Authorization succeeded"))
      << "Server should not record successful authorization";
  ExpectServerRemainsRunningFor(
      std::chrono::duration_cast<std::chrono::milliseconds>(
          kPostConnectStabilityTimeout),
      "waiting for the server to remain stable after rejecting bad credentials");
}

INSTANTIATE_TEST_SUITE_P(
    Protocols,
    ClientServerE2eTest,
    ::testing::Values(E2eProtocol::Remote, E2eProtocol::OpcUa),
    [](const ::testing::TestParamInfo<E2eProtocol>& info) {
      return std::string{ToString(info.param)};
    });

}  // namespace
}  // namespace client::test
