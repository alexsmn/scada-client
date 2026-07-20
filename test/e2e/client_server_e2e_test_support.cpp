#include "test/e2e/client_server_e2e_test_support.h"

#include "test/e2e/iec61850_test_server.h"

#include <boost/asio/connect.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/json.hpp>
#include <boost/process/v1/args.hpp>
#include <boost/process/v1/io.hpp>

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <utility>

using namespace std::chrono_literals;

namespace client::test {
namespace {

bool IsKeepWorkspaceEnabled() {
  auto* value = std::getenv("SCADA_E2E_KEEP_WORKSPACE");
  return value && *value;
}

// Whether the spawned Qt client should render offscreen (no visible window)
// during the E2E run. On by default so the suite does not pop up — and steal
// focus with — a client window for every one of the 40 test cases. Set
// SCADA_E2E_HIDE_CLIENT_WINDOW to 0/false/no/off to show the window when
// debugging a test locally. The client still runs its full UI logic offscreen;
// only the platform windowing is suppressed.
bool IsHideClientWindowEnabled() {
  auto* value = std::getenv("SCADA_E2E_HIDE_CLIENT_WINDOW");
  if (!value || !*value)
    return true;
  std::string normalized{value};
  std::transform(normalized.begin(), normalized.end(), normalized.begin(),
                 [](unsigned char c) { return std::tolower(c); });
  return normalized != "0" && normalized != "false" && normalized != "no" &&
         normalized != "off";
}

constexpr auto kWaitStep = 100ms;
constexpr auto kServerStartTimeout = 30s;
constexpr auto kClientStartTimeout = 30s;
constexpr auto kServerLogTimeout = 10s;
constexpr auto kObjectTreeLoadTimeout = 30s;
// The value/label/device checks all browse the node tree AND then wait for
// slower per-node work: monitored-item current values, DisplayName resolution,
// or device links coming online. That work travels the full aggregation/proxy
// path (materially slower for the Remote/gRPC protocol under Cluster), and each
// one passes comfortably in isolation but can exceed 30s when the whole matrix
// runs sequentially and many tier processes saturate the machine. These waits
// must stay strictly larger than the client-side capture deadlines they observe
// (60s — see e2e_test_support.{h,cpp}); the client writes its report on success
// or at that deadline, and the test waits for that report to appear, so a
// too-short wait here would time out before the client even records its verdict.
constexpr auto kObjectViewValuesTimeout = 75s;
constexpr auto kObjectTreeLabelsTimeout = 75s;
constexpr auto kHardwareTreeDevicesTimeout = 75s;
constexpr auto kOperatorUseCasesTimeout = 30s;
constexpr auto kProfileSaveTimeout = 30s;
constexpr auto kHistoricalTimedDataTimeout = 30s;

// Turns the analog item TIT.4 (which already references the RAMP simulation
// signal {9,3}) into a simulated, historized item collected into the analog
// historical DB {6,2}. Mirrors the server integration suite's value-flow recipe
// so the server accumulates a steady stream of samples the client can read back.
constexpr std::string_view kHistorizeSimulatedItemSql =
    "UPDATE AnalogItemType SET Simulated=1, HasHistoricalDatabaseNS=6, "
    "HasHistoricalDatabaseID=2 WHERE ID=4;";

// The runtime node id of the historized item TIT.4 (analog item id 4 in the TIT
// namespace, index 2). In Cluster mode the historian pull-collects this node
// from the edge that serves it (historyCollection.sources[].nodes), filing the
// samples under its own HasHistoricalDatabase config (the SQL above).
constexpr std::string_view kHistorizedNodeId = "ns=2;i=4";

std::string_view GetServerType(E2eProtocol protocol) {
  switch (protocol) {
    case E2eProtocol::Remote:
      return "Scada";
    case E2eProtocol::OpcUa:
      return "OpcUa";
  }
  return {};
}

std::filesystem::path GetServerExePath() {
  return std::filesystem::path{SCADA_E2E_SERVER_EXE};
}

// Cluster-topology tier binaries (each tier is a distinct executable).
std::filesystem::path GetConfigExePath() {
  return std::filesystem::path{SCADA_E2E_CONFIG_EXE};
}
std::filesystem::path GetHistorianExePath() {
  return std::filesystem::path{SCADA_E2E_HISTORIAN_EXE};
}
std::filesystem::path GetProxyExePath() {
  return std::filesystem::path{SCADA_E2E_PROXY_EXE};
}
std::filesystem::path GetIec104ExePath() {
  return std::filesystem::path{SCADA_E2E_IEC104_EXE};
}
std::filesystem::path GetModbusExePath() {
  return std::filesystem::path{SCADA_E2E_MODBUS_EXE};
}
std::filesystem::path GetIec61850ExePath() {
  return std::filesystem::path{SCADA_E2E_IEC61850_EXE};
}
std::filesystem::path GetFilesystemExePath() {
  return std::filesystem::path{SCADA_E2E_FILESYSTEM_EXE};
}

std::filesystem::path GetClientExePath() {
  return std::filesystem::path{SCADA_E2E_CLIENT_EXE};
}

std::filesystem::path GetServerFixtureDir() {
  return std::filesystem::path{SCADA_E2E_SERVER_FIXTURE_DIR};
}

std::filesystem::path GetServerSettingsTemplatePath() {
  return std::filesystem::path{SCADA_E2E_SERVER_SETTINGS_TEMPLATE};
}

std::filesystem::path GetConfigurationBaseSqlPath() {
  return std::filesystem::path{SCADA_E2E_CONFIGURATION_BASE_SQL};
}

std::filesystem::path GetConfigurationFixtureSqlPath() {
  return std::filesystem::path{SCADA_E2E_CONFIGURATION_FIXTURE_SQL};
}

std::filesystem::path GetSqliteExePath() {
  auto* value = std::getenv("SCADA_SQLITE3_EXE");
  if (value && *value)
    return value;
  return std::filesystem::path{SCADA_E2E_SQLITE3_EXE};
}

// Resolves SCADA_SERVER_LICENSE_FILE against the *test process* working
// directory. Each tier is launched with its own temporary workspace as the
// working directory, so a relative env value would not resolve there and the
// tier would start unlicensed ("License: No license found").
std::filesystem::path GetSignedLicensePath() {
  auto* value = std::getenv("SCADA_SERVER_LICENSE_FILE");
  if (!value || !*value)
    return {};

  std::error_code ec;
  auto absolute = std::filesystem::absolute(std::filesystem::path{value}, ec);
  return ec ? std::filesystem::path{value} : absolute;
}

::testing::AssertionResult ValidateSignedLicenseEnv() {
  const auto license_path = GetSignedLicensePath();
  if (license_path.empty()) {
    return ::testing::AssertionFailure()
           << "SCADA_SERVER_LICENSE_FILE must be set to an external signed "
              "license JSON before running client/server E2E tests";
  }

  if (!std::filesystem::exists(license_path)) {
    return ::testing::AssertionFailure()
           << "SCADA_SERVER_LICENSE_FILE points to missing license file: "
           << license_path;
  }

  return ::testing::AssertionSuccess();
}

void ConfigureSignedLicense(boost::json::object& server_json) {
  const auto license_path = GetSignedLicensePath();
  if (license_path.empty())
    return;

  boost::json::object license{{"file", license_path.string()}};

  if (auto* require_gcp_binding =
          std::getenv("SCADA_SERVER_LICENSE_REQUIRE_GCP_BINDING");
      require_gcp_binding && *require_gcp_binding) {
    license["require_gcp_binding"] =
        std::string_view{require_gcp_binding} == "true" ||
        std::string_view{require_gcp_binding} == "1";
  }

  server_json["license"] = std::move(license);
}

// Binds the shared server-process harness (common/test/e2e) to this suite's
// SCADA_E2E_* paths and its env-var signed license (ConfigureSignedLicense), for
// the given tier binary.
ServerProcessContext MakeServerContextForExe(const std::filesystem::path& exe) {
  return ServerProcessContext{
      .server_exe = exe,
      .fixture_dir = GetServerFixtureDir(),
      .settings_template = GetServerSettingsTemplatePath(),
      .configuration_base_sql = GetConfigurationBaseSqlPath(),
      .configuration_fixture_sql = GetConfigurationFixtureSqlPath(),
      .sqlite_exe = GetSqliteExePath(),
      .configure_license =
          [](boost::json::object& server_json, const std::filesystem::path&) {
            ConfigureSignedLicense(server_json);
          },
  };
}

// The SingleTier server binary bound to the shared harness.
ServerProcessContext MakeServerContext() {
  return MakeServerContextForExe(GetServerExePath());
}

// Full-rights multi-session service account for inter-tier logins. The built-in
// root user is single-session (configuration_authenticator.cpp), so concurrent
// edge→config / edge→historian / proxy→edge logins would collide on
// Bad_UserIsAlreadyLoggedOn; `svc` (id 100, MultiSessions=1) does not. Mirrors
// gcp/free-tier/multitier/configs/seed-svc-user.sql. Seeded into every tier that
// authenticates an inter-tier client (config, historian, and the edges/proxy the
// aggregator logs into); its password is provisioned via security.provision.
constexpr std::string_view kSvcUserSql =
    "INSERT OR REPLACE INTO UserType "
    "(ID, ParentNS, ParentID, BrowseName, DisplayName, AccessRights, "
    "MultiSessions) VALUES (100, 7, 29, 'svc', 'svc', 3, 1);";
constexpr std::string_view kSvcUser = "svc";
constexpr std::string_view kSvcPassword = "svc-e2e-password";

// Appends {id:100, password:svc} to server.json's security.provision[] so the
// tier can authenticate inbound svc logins, preserving any existing entries
// (e.g. the fixture's guest id 12).
void ProvisionSvcPassword(boost::json::object& server_json) {
  auto& security = server_json["security"].is_object()
                       ? server_json["security"].as_object()
                       : server_json["security"].emplace_object();
  auto& provision = security["provision"].is_array()
                        ? security["provision"].as_array()
                        : security["provision"].emplace_array();
  provision.push_back(boost::json::object{
      {"id", 100}, {"password", std::string{kSvcPassword}}});
}

int FindAvailablePort() {
  boost::asio::io_context io_context;
  boost::asio::ip::tcp::acceptor acceptor{
      io_context,
      boost::asio::ip::tcp::endpoint{boost::asio::ip::tcp::v4(), 0}};
  return static_cast<int>(acceptor.local_endpoint().port());
}

bool CanConnectTcp(int port) {
  try {
    boost::asio::io_context io_context;
    boost::asio::ip::tcp::resolver resolver{io_context};
    boost::asio::ip::tcp::socket socket{io_context};
    auto endpoints = resolver.resolve("127.0.0.1", std::to_string(port));
    boost::asio::connect(socket, endpoints);
    return true;
  } catch (...) {
    return false;
  }
}

std::string RunSqliteScalar(const std::filesystem::path& database_path,
                            std::string sql) {
  process::ipstream output;
  process::child sqlite{GetSqliteExePath().string(),
                        process::args({"-batch", "-noheader",
                                       database_path.string(), std::move(sql)}),
                        process::std_out > output,
                        process::std_err > process::null};

  std::string result;
  std::string line;
  while (std::getline(output, line)) {
    if (!result.empty())
      result += '\n';
    result += line;
  }

  sqlite.wait();
  if (sqlite.exit_code() != 0) {
    throw std::runtime_error{"sqlite3 query failed for " +
                             database_path.string()};
  }
  return result;
}

template <class Predicate>
bool WaitUntil(Predicate&& predicate,
               std::chrono::milliseconds timeout,
               std::chrono::milliseconds step = kWaitStep) {
  auto deadline = std::chrono::steady_clock::now() + timeout;
  while (std::chrono::steady_clock::now() < deadline) {
    if (predicate())
      return true;
    std::this_thread::sleep_for(step);
  }
  return predicate();
}

}  // namespace

const std::chrono::seconds kPostConnectStabilityTimeout = 10s;
const std::string_view kStartupCompletedLog =
    "Client startup completed; entering steady-state run loop";

std::string_view ToString(E2eProtocol protocol) {
  switch (protocol) {
    case E2eProtocol::Remote:
      return "Remote";
    case E2eProtocol::OpcUa:
      return "OpcUa";
  }
  return "Unknown";
}

std::string_view ToString(ServerTopology topology) {
  switch (topology) {
    case ServerTopology::SingleTier:
      return "SingleTier";
    case ServerTopology::Cluster:
      return "Cluster";
  }
  return "Unknown";
}

std::string E2eParamName(const E2eParam& param) {
  return std::string{ToString(param.protocol)} + "_" +
         std::string{ToString(param.topology)};
}

ClientServerE2eTest::ClientServerE2eTest()
    : job_{std::make_unique<JobObject>()} {}

ClientServerE2eTest::~ClientServerE2eTest() = default;

void ClientServerE2eTest::SetUp() {
  ASSERT_TRUE(std::filesystem::exists(GetServerExePath()));
  ASSERT_TRUE(std::filesystem::exists(GetClientExePath()));
  ASSERT_TRUE(std::filesystem::exists(GetServerFixtureDir()));
  ASSERT_TRUE(std::filesystem::exists(GetServerSettingsTemplatePath()));
  ASSERT_TRUE(std::filesystem::exists(GetConfigurationBaseSqlPath()));
  ASSERT_TRUE(std::filesystem::exists(GetConfigurationFixtureSqlPath()));
  ASSERT_TRUE(ValidateSignedLicenseEnv());

  remote_port_ = FindAvailablePort();
  opcua_port_ = FindAvailablePort();
  while (opcua_port_ == remote_port_)
    opcua_port_ = FindAvailablePort();
  iec61850_port_ = FindAvailablePort();
  while (iec61850_port_ == remote_port_ || iec61850_port_ == opcua_port_)
    iec61850_port_ = FindAvailablePort();
  // Reserve the client-facing (proxy / single-tier) ports and the IEC 61850 port
  // so the shared PortPool hands the cluster tiers distinct ports in Cluster
  // mode.
  for (int port : {remote_port_, opcua_port_, iec61850_port_})
    ports_.Reserve(port);
  PrepareWorkspace();

  iec61850_server_ = std::make_unique<Iec61850TestServer>(iec61850_port_);
  ASSERT_TRUE(WaitUntil(
      [this] {
        return iec61850_server_->running() || iec61850_server_->failed();
      },
      5s));
  ASSERT_FALSE(iec61850_server_->failed());
}

void ClientServerE2eTest::TearDown() {
  // Tiers in reverse dependency order: proxy/edges before the config/historian
  // they depend on.
  ServerTier* const tiers[] = {iec104_tier_.get(),     modbus_tier_.get(),
                               iec61850_tier_.get(),   filesystem_tier_.get(),
                               historian_tier_.get(),  config_tier_.get()};
  if (HasFailure() || IsKeepWorkspaceEnabled()) {
    workspace_.Preserve();
    for (ServerTier* tier : tiers) {
      if (tier)
        tier->PreserveWorkspace();
    }
    std::cerr << "Preserved E2E workspace: " << workspace_.path() << '\n';
  }
  job_->Terminate();
  ForceTerminate(client_);
  ForceTerminate(server_);
  for (ServerTier* tier : tiers) {
    if (tier)
      tier->Terminate();
  }
  WaitForExit(client_);
  WaitForExit(server_);
  iec61850_server_.reset();
}

void ClientServerE2eTest::PrepareServerFilesystem(
    const std::filesystem::path& ws,
    int iec61850_port) {
  std::filesystem::copy(GetServerFixtureDir(), ws,
                        std::filesystem::copy_options::recursive |
                            std::filesystem::copy_options::overwrite_existing);
  GenerateConfigurationDatabase(MakeServerContext(), ws, iec61850_port);
}

void ClientServerE2eTest::WriteServerJson(
    const std::filesystem::path& ws,
    int remote_port,
    int opcua_port,
    const std::function<void(boost::json::object&)>& configure) {
  auto server_json_value =
      boost::json::parse(ReadFileOrEmpty(GetServerSettingsTemplatePath()));
  auto& server_json = server_json_value.as_object();
  ConfigureSignedLicense(server_json);
  server_json["sessions"] = boost::json::array{
      "tcp;passive;host=0.0.0.0;port=" + std::to_string(remote_port)};
  auto& opcua = server_json["opcua"].is_object()
                    ? server_json["opcua"].as_object()
                    : server_json["opcua"].emplace_object();
  opcua["enabled"] = true;
  opcua["url"] =
      boost::json::array{"opc.tcp://127.0.0.1:" + std::to_string(opcua_port)};
  opcua["trace"] = "none";
  if (configure)
    configure(server_json);
  WriteTextFile(ws / "server.json", boost::json::serialize(server_json_value));
}

void ClientServerE2eTest::PrepareWorkspace() {
  PrepareServerFilesystem(workspace_.path(), iec61850_port_);
  WriteServerJson(workspace_.path(), remote_port_, opcua_port_);

  status_file_ = workspace_.path() / "client-status.txt";
  object_view_values_file_ = workspace_.path() / "object-view-values.txt";
  object_tree_labels_file_ = workspace_.path() / "object-tree-labels.txt";
  hardware_tree_devices_file_ = workspace_.path() / "hardware-tree-devices.txt";
  operator_use_cases_file_ = workspace_.path() / "operator-use-cases.txt";
  profile_save_file_ = workspace_.path() / "profile-save.txt";
  historical_timed_data_file_ = workspace_.path() / "historical-timed-data.txt";
  settings_file_ = workspace_.path() / "client-settings.json";
  server_log_dir_ = workspace_.path() / "Logs";
  client_log_dir_ = workspace_.path() / "ClientLogs";
}

void ClientServerE2eTest::WriteClientSettings(std::string_view password,
                                              std::string_view user,
                                              std::string_view security_mode) {
  const auto remote_host =
      std::string{"localhost:"} + std::to_string(remote_port_);
  const auto opcua_host =
      std::string{"127.0.0.1:"} + std::to_string(opcua_port_);
  boost::json::object root{
      {"ServerType", std::string{GetServerType(Protocol())}},
      {"Host:Scada", remote_host},
      {"Host:OpcUa", opcua_host},
      {"User", std::string{user}},
      {"Password", std::string{password}},
      {"AutoLogin", true},
  };
  if (!security_mode.empty())
    root["SecurityMode"] = std::string{security_mode};
  WriteTextFile(settings_file_, boost::json::serialize(root));
}

void ClientServerE2eTest::EnableSimulatedHistory() {
  historize_simulated_item_ = true;
}

ServerProcessContext ClientServerE2eTest::MakeTierContext(
    const std::filesystem::path& exe) const {
  return MakeServerContextForExe(exe);
}

void ClientServerE2eTest::StartServer() {
  if (Topology() == ServerTopology::Cluster) {
    StartCluster();
    return;
  }

  // The single tier owns the data items directly; historize before the process
  // starts so the data collector picks it up from its config DB at startup.
  if (historize_simulated_item_) {
    ExecuteConfigurationSql(MakeServerContext(), workspace_.path(),
                            std::string{kHistorizeSimulatedItemSql});
  }

  LaunchProcess(GetServerExePath(),
                {"--param=" + (workspace_.path() / "server.json").string()},
                workspace_.path(), *job_, server_);

  const int port = GetProtocolPort();
  ASSERT_TRUE(WaitUntil([port] { return CanConnectTcp(port); },
                        std::chrono::duration_cast<std::chrono::milliseconds>(
                            kServerStartTimeout)))
      << "Server did not start listening on " << ToString(Protocol())
      << " port " << port;
}

void ClientServerE2eTest::StartCluster() {
  // --- Config tier -----------------------------------------------------------
  // Owns the configuration namespace (devices, data items, users, filesystem)
  // that the edges read and re-expose through aggregation. Keeps its generated
  // local config DB plus the svc account the edges authenticate with, and serves
  // the per-protocol device config. The IEC 61850 device port (the shared test
  // server) and the TIT.4 historization live here because the edges read their
  // config from here. Mirrors gcp/free-tier/multitier/configs/config.json.
  config_tier_ =
      std::make_unique<ServerTier>(MakeTierContext(GetConfigExePath()));
  config_tier_->AllocatePorts(ports_);
  std::string config_sql{kSvcUserSql};
  if (historize_simulated_item_)
    config_sql += std::string{kHistorizeSimulatedItemSql};
  config_tier_->Launch(ServerTier::Options{
      .configure =
          [](boost::json::object& json) {
            json["deviceConfig"] = boost::json::object{};
            // Serves config only — no protocol drivers of its own, and no file
            // store (the filesystem tier owns the FileSystem subtree).
            json.erase("iec60870");
            json.erase("modbus");
            json.erase("iec61850");
            json.erase("filesystem");
            ProvisionSvcPassword(json);
          },
      .iec61850_port = iec61850_port_,
      .extra_config_sql = config_sql,
  });
  ASSERT_TRUE(config_tier_->WaitListening())
      << "cluster config tier did not start listening on OPC UA port "
      << config_tier_->opcua_port();

  // --- Historian tier (create + allocate ports now; launched below once the
  //     edges' ports are known so it can pull-collect from one) ---------------
  // Owns the history store. In the history test it pull-collects the historized
  // TIT.4 from the edge that serves it (historyCollection.sources — the ADR 0002
  // subscription model) and files the samples under its own HasHistoricalDatabase
  // config. Mirrors gcp/free-tier/multitier/configs/historian.json.
  historian_tier_ =
      std::make_unique<ServerTier>(MakeTierContext(GetHistorianExePath()));
  historian_tier_->AllocatePorts(ports_);
  const std::string config_url = config_tier_->OpcUaUrl();
  const std::string historian_url = historian_tier_->OpcUaUrl();

  // --- Device edges (create + allocate ports now; launched after the historian
  //     so its historyCollection can reference an edge, while the edges'
  //     history.endpoint references the historian — the two links cross) ------
  // Each edge is a config client (no local DB) running exactly one driver,
  // fetching config from the config tier and routing history to the historian,
  // both as svc. The proxy aggregates them anonymously. Mirrors the GCP
  // configs/{iec104,modbus,iec61850}.json edges.
  struct EdgeSpec {
    std::unique_ptr<ServerTier>* slot;
    std::filesystem::path exe;
    std::string_view driver;
  };
  const EdgeSpec edges[] = {
      {&iec104_tier_, GetIec104ExePath(), "iec60870"},
      {&modbus_tier_, GetModbusExePath(), "modbus"},
      {&iec61850_tier_, GetIec61850ExePath(), "iec61850"},
  };
  for (const EdgeSpec& edge : edges) {
    *edge.slot = std::make_unique<ServerTier>(MakeTierContext(edge.exe));
    (*edge.slot)->AllocatePorts(ports_);
  }
  // The iec104 edge serves the historized TIT.4; the historian pulls it from
  // there (any edge would do — all serve the config-derived data items).
  const std::string collect_source_url = iec104_tier_->OpcUaUrl();

  std::string historian_sql{kSvcUserSql};
  if (historize_simulated_item_)
    historian_sql += std::string{kHistorizeSimulatedItemSql};
  historian_tier_->Launch(ServerTier::Options{
      .configure =
          [collect_source_url,
           historize = historize_simulated_item_](boost::json::object& json) {
            json.erase("iec60870");
            json.erase("modbus");
            json.erase("iec61850");
            json.erase("filesystem");
            ProvisionSvcPassword(json);
            if (historize) {
              json["historyCollection"] = boost::json::object{
                  {"sources",
                   boost::json::array{boost::json::object{
                       {"endpoint", collect_source_url},
                       {"user", std::string{kSvcUser}},
                       {"password", std::string{kSvcPassword}},
                       {"nodes", boost::json::array{
                                     std::string{kHistorizedNodeId}}}}}}};
            }
          },
      .extra_config_sql = historian_sql,
  });
  ASSERT_TRUE(historian_tier_->WaitListening())
      << "cluster historian tier did not start listening on OPC UA port "
      << historian_tier_->opcua_port();

  auto make_edge_configure = [config_url, historian_url](
                                 std::string_view keep_driver) {
    return [config_url, historian_url,
            keep_driver](boost::json::object& json) {
      for (std::string_view driver : {"iec60870", "modbus", "iec61850"}) {
        if (driver != keep_driver)
          json.erase(driver);
      }
      // The filesystem tier exclusively owns the FileSystem subtree; an edge
      // running its own file store would merge a second tree into the proxy's
      // fan-out Browse.
      json.erase("filesystem");
      json["configuration"] = boost::json::object{
          {"endpoint", config_url},
          {"user", std::string{kSvcUser}},
          {"password", std::string{kSvcPassword}}};
      json["history"] = boost::json::object{
          {"endpoint", historian_url},
          {"user", std::string{kSvcUser}},
          {"password", std::string{kSvcPassword}}};
    };
  };
  for (const EdgeSpec& edge : edges) {
    (*edge.slot)->Launch(ServerTier::Options{
        .configure = make_edge_configure(edge.driver),
        .remove_local_config_db = true,
    });
  }
  for (const EdgeSpec& edge : edges) {
    ASSERT_TRUE((*edge.slot)->WaitListening())
        << "cluster " << edge.driver
        << " edge did not start listening on OPC UA port "
        << (*edge.slot)->opcua_port();
  }

  // --- Filesystem tier -------------------------------------------------------
  // The dedicated file store: serves the FileSystem subtree from its own
  // workspace; no drivers/history/data items. It keeps a LOCAL config DB (like
  // config/historian/proxy) because it must authenticate the proxy's svc
  // aggregation login — anonymous sessions are denied the forwarded
  // AddNodes/DeleteNodes, and remote-config tiers cannot resolve non-root
  // users yet (the known LoadNodes(UserType) gap). Its data-item rows are
  // stripped like the proxy's so the edges stay authoritative for values. The
  // proxy exclusively claims the file namespace + root to it below. Mirrors
  // gcp/free-tier/multitier/configs/filesystem.json.
  filesystem_tier_ =
      std::make_unique<ServerTier>(MakeTierContext(GetFilesystemExePath()));
  filesystem_tier_->AllocatePorts(ports_);
  filesystem_tier_->Launch(ServerTier::Options{
      .configure =
          [](boost::json::object& json) {
            json.erase("iec60870");
            json.erase("modbus");
            json.erase("iec61850");
            json["dataItems"] = boost::json::object{{"enabled", false}};
            json["history"] = boost::json::object{{"enabled", false}};
            ProvisionSvcPassword(json);
            // The template's filesystem block stays: it roots the store at this
            // tier's own ${DIR_PARAM}/FileSystem workspace dir.
          },
      .extra_config_sql = std::string{kSvcUserSql} +
                          "PRAGMA foreign_keys=OFF;\n"
                          "DELETE FROM AnalogItemType;\n"
                          "DELETE FROM DiscreteItemType;\n",
  });
  ASSERT_TRUE(filesystem_tier_->WaitListening())
      << "cluster filesystem tier did not start listening on OPC UA port "
      << filesystem_tier_->opcua_port();

  // --- Proxy (client-facing) -------------------------------------------------
  // Aggregates the three edges anonymously (mirrors the GCP proxy.json). Reuses
  // the built-in server_ slot on the client-facing ports and keeps workspace_'s
  // local config DB to authenticate the client's login; data-item module off and
  // its data-item rows stripped so the aggregated edge namespace is authoritative.
  ExecuteConfigurationSql(MakeServerContextForExe(GetProxyExePath()),
                          workspace_.path(),
                          "PRAGMA foreign_keys=OFF;\n"
                          "DELETE FROM AnalogItemType;\n"
                          "DELETE FROM DiscreteItemType;\n");
  boost::json::array aggregation_servers;
  for (const EdgeSpec& edge : edges) {
    aggregation_servers.push_back(
        boost::json::object{{"endpoint", (*edge.slot)->OpcUaUrl()}});
  }
  // The file-store downstream. It no longer needs a namespace claim: the file
  // instance namespace is now tier-exclusive (ADR 0003 — only this tier
  // publishes FILESYSTEM_FILE), so the proxy routes it there by ownership. The
  // one remaining claim is the FileSystem root object i=304, which lives in the
  // shared SCADA namespace (every tier serves it) and so cannot be routed by
  // namespace alone — it anchors top-level AddNodes to this tier. Its
  // model-change events are re-raised to the proxy's clients; the link presents
  // svc, since file create/delete forwarding needs a non-anonymous downstream
  // session under enforce_permissions. Mirrors the GCP proxy.json entry.
  aggregation_servers.push_back(boost::json::object{
      {"endpoint", filesystem_tier_->OpcUaUrl()},
      {"user", std::string{kSvcUser}},
      {"password", std::string{kSvcPassword}},
      {"nodes", boost::json::array{"ns=7;i=304"}},
      {"forward_events", true}});
  WriteServerJson(
      workspace_.path(), remote_port_, opcua_port_,
      [&aggregation_servers](boost::json::object& server_json) {
        server_json.erase("iec60870");
        server_json.erase("modbus");
        server_json.erase("iec61850");
        // The filesystem tier owns the FileSystem subtree; the proxy must not
        // run a local file store of its own.
        server_json.erase("filesystem");
        server_json["dataItems"] = boost::json::object{{"enabled", false}};
        server_json["aggregation"] =
            boost::json::object{{"servers", aggregation_servers}};
      });
  LaunchProcess(GetProxyExePath(),
                {"--param=" + (workspace_.path() / "server.json").string()},
                workspace_.path(), *job_, server_);

  const int port = GetProtocolPort();
  ASSERT_TRUE(WaitUntil([port] { return CanConnectTcp(port); },
                        std::chrono::duration_cast<std::chrono::milliseconds>(
                            kServerStartTimeout)))
      << "cluster proxy did not start listening on " << ToString(Protocol())
      << " port " << port;
}

void ClientServerE2eTest::StartClient(std::vector<std::string> extra_args) {
  std::vector<std::string> args{
      "--test-settings-file=" + settings_file_.string(),
      "--test-status-file=" + status_file_.string(),
      "--test-log-dir=" + client_log_dir_.string()};
  args.insert(args.end(), std::make_move_iterator(extra_args.begin()),
              std::make_move_iterator(extra_args.end()));

  std::vector<std::pair<std::string, std::string>> extra_env;
  if (IsHideClientWindowEnabled()) {
    // Qt reads QT_QPA_PLATFORM when QApplication is constructed; "offscreen"
    // runs the client without ever mapping a window on screen.
    extra_env.emplace_back("QT_QPA_PLATFORM", "offscreen");
  }

  LaunchProcess(GetClientExePath(), args, GetClientExePath().parent_path(),
                *job_, client_, extra_env);
}

std::string ClientServerE2eTest::WaitForStatus() {
  bool ok = WaitUntil(
      [this] {
        return std::filesystem::exists(status_file_) || !client_.IsRunning();
      },
      std::chrono::duration_cast<std::chrono::milliseconds>(
          kClientStartTimeout));
  EXPECT_TRUE(ok) << "Timed out waiting for client status file";
  return ReadFileOrEmpty(status_file_);
}

bool ClientServerE2eTest::WaitForStartupOrStatus() {
  return WaitUntil(
      [this] {
        return ContainsInDirectory(client_log_dir_, kStartupCompletedLog) ||
               std::filesystem::exists(status_file_) || !client_.IsRunning();
      },
      std::chrono::duration_cast<std::chrono::milliseconds>(
          kClientStartTimeout));
}

bool ClientServerE2eTest::WaitForObjectTreeReady() {
  return WaitUntil(
      [this] {
        auto child_count = FindLoggedObjectTreeChildCount(client_log_dir_);
        return (child_count && *child_count > 0) || !client_.IsRunning();
      },
      std::chrono::duration_cast<std::chrono::milliseconds>(
          kObjectTreeLoadTimeout));
}

std::string ClientServerE2eTest::WaitForObjectViewValuesReport() {
  bool ok = WaitUntil(
      [this] {
        return std::filesystem::exists(object_view_values_file_) ||
               !client_.IsRunning();
      },
      std::chrono::duration_cast<std::chrono::milliseconds>(
          kObjectViewValuesTimeout));
  EXPECT_TRUE(ok) << "Timed out waiting for object-view values report";
  return ReadFileOrEmpty(object_view_values_file_);
}

std::string ClientServerE2eTest::WaitForObjectTreeLabelsReport() {
  bool ok = WaitUntil(
      [this] {
        return std::filesystem::exists(object_tree_labels_file_) ||
               !client_.IsRunning();
      },
      std::chrono::duration_cast<std::chrono::milliseconds>(
          kObjectTreeLabelsTimeout));
  EXPECT_TRUE(ok) << "Timed out waiting for object-tree labels report";
  return ReadFileOrEmpty(object_tree_labels_file_);
}

std::string ClientServerE2eTest::WaitForHardwareTreeDevicesReport() {
  bool ok = WaitUntil(
      [this] {
        return std::filesystem::exists(hardware_tree_devices_file_) ||
               !client_.IsRunning();
      },
      std::chrono::duration_cast<std::chrono::milliseconds>(
          kHardwareTreeDevicesTimeout));
  EXPECT_TRUE(ok) << "Timed out waiting for hardware-tree devices report";
  return ReadFileOrEmpty(hardware_tree_devices_file_);
}

std::string ClientServerE2eTest::WaitForOperatorUseCasesReport() {
  bool ok = WaitUntil(
      [this] {
        return std::filesystem::exists(operator_use_cases_file_) ||
               !client_.IsRunning();
      },
      std::chrono::duration_cast<std::chrono::milliseconds>(
          kOperatorUseCasesTimeout));
  EXPECT_TRUE(ok) << "Timed out waiting for operator use-case report";
  return ReadFileOrEmpty(operator_use_cases_file_);
}

std::string ClientServerE2eTest::WaitForProfileSaveReport() {
  bool ok = WaitUntil(
      [this] {
        return std::filesystem::exists(profile_save_file_) ||
               !client_.IsRunning();
      },
      std::chrono::duration_cast<std::chrono::milliseconds>(
          kProfileSaveTimeout));
  EXPECT_TRUE(ok) << "Timed out waiting for profile-save report";
  return ReadFileOrEmpty(profile_save_file_);
}

std::string ClientServerE2eTest::WaitForHistoricalTimedDataReport() {
  bool ok = WaitUntil(
      [this] {
        return std::filesystem::exists(historical_timed_data_file_) ||
               !client_.IsRunning();
      },
      std::chrono::duration_cast<std::chrono::milliseconds>(
          kHistoricalTimedDataTimeout));
  EXPECT_TRUE(ok) << "Timed out waiting for historical timed-data report";
  return ReadFileOrEmpty(historical_timed_data_file_);
}

std::filesystem::path ClientServerE2eTest::ServerConfigDatabasePath() const {
  // In Cluster mode the client writes profiles through the proxy, which routes
  // the config write to the config tier that owns the aggregated config
  // namespace, so the profile lands in the config tier's DB (not the proxy's).
  const std::filesystem::path& ws =
      Topology() == ServerTopology::Cluster && config_tier_
          ? config_tier_->WorkspaceDir()
          : workspace_.path();
  return ws / "Configuration" / "configuration.sqlite3";
}

std::string ClientServerE2eTest::ReadUserProfileJsonFromServerDatabase(
    int user_id) {
  return RunSqliteScalar(
      ServerConfigDatabasePath(),
      "SELECT COALESCE(ProfileJson, '') FROM UserType WHERE ID = " +
          std::to_string(user_id) + ";");
}

std::string ClientServerE2eTest::ReadUserProfileRevisionFromServerDatabase(
    int user_id) {
  return RunSqliteScalar(
      ServerConfigDatabasePath(),
      "SELECT COALESCE(ProfileRevision, '') FROM UserType WHERE ID = " +
          std::to_string(user_id) + ";");
}

std::string ClientServerE2eTest::DescribeProcessExit(
    const ChildProcess& process,
    std::string_view name) const {
  auto exit_code = process.ExitCode();
  if (!exit_code)
    return std::string{name} + " process exit code unavailable";
  if (process.IsRunning())
    return std::string{name} + " is still running";
  return std::string{name} + " exited with code " + std::to_string(*exit_code);
}

void ClientServerE2eTest::ExpectProcessesRemainRunningFor(
    std::chrono::milliseconds timeout,
    std::string_view context) {
  auto ok = WaitUntil(
      [this] { return !server_.IsRunning() || !client_.IsRunning(); }, timeout);
  EXPECT_FALSE(ok) << "Unexpected process exit while " << context << ": "
                   << DescribeProcessExit(server_, "server") << ", "
                   << DescribeProcessExit(client_, "client");
}

void ClientServerE2eTest::ExpectServerRemainsRunningFor(
    std::chrono::milliseconds timeout,
    std::string_view context) {
  auto ok = WaitUntil([this] { return !server_.IsRunning(); }, timeout);
  EXPECT_FALSE(ok) << "Unexpected server exit while " << context << ": "
                   << DescribeProcessExit(server_, "server");
}

void ClientServerE2eTest::ExpectServerAuthLog() {
  EXPECT_TRUE(WaitUntil(
      [this] {
        switch (Protocol()) {
          case E2eProtocol::Remote:
            return ContainsInDirectory(server_log_dir_,
                                       "Authorization succeeded") ||
                   ContainsInDirectory(server_log_dir_,
                                       "CreateSession completed");
          case E2eProtocol::OpcUa:
            return ContainsInDirectory(server_log_dir_,
                                       "OPC UA session activated");
        }
        return false;
      },
      std::chrono::duration_cast<std::chrono::milliseconds>(kServerLogTimeout)))
      << "Server logs did not record a successful session in "
      << server_log_dir_;
}

int ClientServerE2eTest::GetProtocolPort() const {
  switch (Protocol()) {
    case E2eProtocol::Remote:
      return remote_port_;
    case E2eProtocol::OpcUa:
      return opcua_port_;
  }
  return 0;
}

}  // namespace client::test
