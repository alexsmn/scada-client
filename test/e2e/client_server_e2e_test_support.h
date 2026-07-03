#pragma once

#include "test/e2e/e2e_file_helpers.h"
#include "test/e2e/e2e_process.h"
#include "test/e2e/e2e_server_process.h"

#include <gtest/gtest.h>

#include <chrono>
#include <filesystem>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace boost::json {
class object;
}

namespace client::test {

class Iec61850TestServer;

enum class E2eProtocol {
  Remote,
  OpcUa,
};

// Which server backend the client is exercised against:
//   Monolith     — a single server process (the default).
//   MultiProcess — the tier-split server: a full-content edge fronted by an
//                  aggregating proxy, with the client connecting to the proxy
//                  (mirrors server/dev/local-cluster). Verifies the client
//                  behaves identically whether the server is one process or a
//                  multi-process cluster behind a northbound proxy.
enum class ServerTopology {
  Monolith,
  MultiProcess,
};

std::string_view ToString(E2eProtocol protocol);
std::string_view ToString(ServerTopology topology);

// One point in the E2E parameter space: protocol × server topology.
struct E2eParam {
  E2eProtocol protocol;
  ServerTopology topology;
};

// Test-name suffix for a parameter, e.g. "Remote_Monolith" /
// "OpcUa_MultiProcess".
std::string E2eParamName(const E2eParam& param);

extern const std::chrono::seconds kPostConnectStabilityTimeout;
extern const std::string_view kStartupCompletedLog;

class ClientServerE2eTest : public ::testing::TestWithParam<E2eParam> {
 protected:
  ClientServerE2eTest();
  ~ClientServerE2eTest() override;

  void SetUp() override;
  void TearDown() override;

  E2eProtocol Protocol() const { return GetParam().protocol; }
  ServerTopology Topology() const { return GetParam().topology; }

  void PrepareWorkspace();
  // Writes the client settings file. `security_mode`, when non-empty, sets the
  // OPC UA endpoint security selection ("None" / "Auto" / "SignAndEncrypt");
  // it is ignored by the Scada (gRPC) backend.
  void WriteClientSettings(std::string_view password,
                           std::string_view user = "root",
                           std::string_view security_mode = {});
  // Historizes + simulates the analog item TIT.4 so the server collects a steady
  // stream of samples the client's timed-data view can read back. Call before
  // StartServer(); applies to the monolith's DB, or the edge's in MultiProcess.
  void EnableSimulatedHistory();
  void StartServer();
  void StartClient(std::vector<std::string> extra_args = {});

  std::string WaitForStatus();
  bool WaitForStartupOrStatus();
  bool WaitForObjectTreeReady();
  std::string WaitForObjectViewValuesReport();
  std::string WaitForObjectTreeLabelsReport();
  std::string WaitForHardwareTreeDevicesReport();
  std::string WaitForOperatorUseCasesReport();
  std::string WaitForProfileSaveReport();
  std::string WaitForHistoricalTimedDataReport();
  std::string ReadUserProfileJsonFromServerDatabase(int user_id);
  std::string ReadUserProfileRevisionFromServerDatabase(int user_id);

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
  std::filesystem::path profile_save_file_;
  std::filesystem::path historical_timed_data_file_;
  std::filesystem::path settings_file_;
  std::filesystem::path server_log_dir_;
  std::filesystem::path client_log_dir_;

  ChildProcess server_;
  ChildProcess client_;

  // Multi-process topology only: the aggregated downstream edge, launched with
  // the shared ServerTier harness (common/test/e2e). The client-facing proxy
  // reuses the built-in server_/workspace_/remote_port_/opcua_port_ slot, so
  // every existing assertion (auth logs, stability, profile DB reads) targets
  // the process the client actually connects to.
  PortPool ports_;
  std::unique_ptr<ServerTier> edge_tier_;

  // Set by EnableSimulatedHistory(); consumed at server launch to historize a
  // simulated item in whichever tier owns the data items.
  bool historize_simulated_item_ = false;

 private:
  int GetProtocolPort() const;

  // Path of the configuration SQLite DB that backs the server nodes the client
  // sees. In Monolith mode that is the single server's DB; in MultiProcess mode
  // the client talks to the proxy but the config (users/profiles) it aggregates
  // is owned by the edge, so profile writes land in the edge's DB.
  std::filesystem::path ServerConfigDatabasePath() const;

  // Copies the server fixture into `ws` and generates its config DB (with the
  // shared IEC 61850 test-server port).
  void PrepareServerFilesystem(const std::filesystem::path& ws,
                               int iec61850_port);
  // Writes `ws`/server.json from the shared template with the given session and
  // OPC UA ports and the signed license, then applies `configure` (when set) to
  // shape the process role (e.g. turn it into an aggregating proxy).
  void WriteServerJson(
      const std::filesystem::path& ws,
      int remote_port,
      int opcua_port,
      const std::function<void(boost::json::object&)>& configure = {});
  // Launches the tier-split cluster: a full-content edge and an aggregating
  // proxy in front of it, with the proxy on the client-facing ports. Waits for
  // the proxy to listen on the active protocol's port.
  void StartMultiProcessCluster();
};

}  // namespace client::test
