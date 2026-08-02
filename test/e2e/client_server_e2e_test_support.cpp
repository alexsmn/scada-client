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
#include <optional>
#include <string>
#include <string_view>
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
// too-short wait here would time out before the client even records its
// verdict.
constexpr auto kObjectViewValuesTimeout = 75s;
constexpr auto kObjectTreeLabelsTimeout = 75s;
constexpr auto kHardwareTreeDevicesTimeout = 75s;
constexpr auto kOperatorUseCasesTimeout = 30s;
constexpr auto kProfileSaveTimeout = 30s;
constexpr auto kHistoricalTimedDataTimeout = 30s;

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

// Binds the shared server-process harness (common/test/e2e) to this suite's
// SCADA_E2E_* paths and the shared env-var signed license
// (ConfigureSignedLicenseFromEnv), for the given tier binary.
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
            ConfigureSignedLicenseFromEnv(server_json);
          },
  };
}

// The SingleTier server binary bound to the shared harness.
ServerProcessContext MakeServerContext() {
  return MakeServerContextForExe(GetServerExePath());
}

// OTLP/gRPC endpoint every process of the run exports to, from
// SCADA_E2E_OTLP_ENDPOINT (e.g. "localhost:4317"). Unset — the CI default —
// leaves the suite exactly as it was: no traces, no OTLP logs, and the metrics
// module's exporter aimed nowhere. Set it to inspect a run in a local
// all-signal viewer; see docs/ops/e2e-client-server.md, "Viewing a run's
// telemetry".
std::string GetOtlpEndpoint() {
  auto* value = std::getenv("SCADA_E2E_OTLP_ENDPOINT");
  return value ? std::string{value} : std::string{};
}

int FindAvailablePort() {
  boost::asio::io_context io_context;
  boost::asio::ip::tcp::acceptor acceptor{
      io_context,
      boost::asio::ip::tcp::endpoint{boost::asio::ip::tcp::v4(), 0}};
  return static_cast<int>(acceptor.local_endpoint().port());
}

bool CanConnectTcp(std::string_view host, std::string_view service) {
  try {
    boost::asio::io_context io_context;
    boost::asio::ip::tcp::resolver resolver{io_context};
    boost::asio::ip::tcp::socket socket{io_context};
    auto endpoints = resolver.resolve(host, service);
    boost::asio::connect(socket, endpoints);
    return true;
  } catch (...) {
    return false;
  }
}

bool CanConnectTcp(int port) {
  return CanConnectTcp("127.0.0.1", std::to_string(port));
}

// Splits a "host:port" endpoint. Returns false when there is no port part.
bool SplitHostPort(std::string_view endpoint,
                   std::string* host,
                   std::string* port) {
  const auto colon = endpoint.rfind(':');
  if (colon == std::string_view::npos || colon + 1 == endpoint.size())
    return false;
  *host = std::string{endpoint.substr(0, colon)};
  *port = std::string{endpoint.substr(colon + 1)};
  return true;
}

bool CanConnectTcpEndpoint(std::string_view endpoint) {
  std::string host;
  std::string port;
  if (!SplitHostPort(endpoint, &host, &port))
    return false;
  return CanConnectTcp(host, port);
}

std::string GetEnvOr(const char* name, std::string fallback) {
  auto* value = std::getenv(name);
  return value && *value ? std::string{value} : std::move(fallback);
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

}  // namespace

const ExternalServerTarget* GetExternalServerTarget() {
  // Resolved once: the environment cannot change mid-run, and every test asks.
  static const std::optional<ExternalServerTarget> target =
      []() -> std::optional<ExternalServerTarget> {
    auto* host = std::getenv("SCADA_E2E_EXTERNAL_HOST");
    if (!host || !*host)
      return std::nullopt;
    return ExternalServerTarget{
        .remote_host = host,
        .opcua_host = GetEnvOr("SCADA_E2E_EXTERNAL_OPCUA_HOST", {}),
        .user = GetEnvOr("SCADA_E2E_EXTERNAL_USER", "root"),
        .password = GetEnvOr("SCADA_E2E_EXTERNAL_PASSWORD", {}),
    };
  }();
  return target ? &*target : nullptr;
}

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
  if (const auto* target = GetExternalServerTarget()) {
    // An external deployment is a fixed topology behind its own client-facing
    // endpoint: it cannot be re-launched per parameter, so the matrix collapses
    // to the parameters it can actually serve. The tier split behind an
    // aggregating proxy is what a deployment is, hence Cluster.
    if (Topology() != ServerTopology::Cluster)
      GTEST_SKIP() << "external server target is a deployed cluster; the "
                      "SingleTier parameter has no external equivalent";
    if (Protocol() == E2eProtocol::OpcUa && target->opcua_host.empty())
      GTEST_SKIP() << "external server target exposes no OPC UA endpoint (set "
                      "SCADA_E2E_EXTERNAL_OPCUA_HOST)";
    ASSERT_TRUE(std::filesystem::exists(GetClientExePath()));
    PrepareWorkspace();
    return;
  }

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
  // Reserve the client-facing (proxy / single-tier) ports and the IEC 61850
  // port so the shared PortPool hands the cluster tiers distinct ports in
  // Cluster mode.
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
  if (HasFailure() || IsKeepWorkspaceEnabled()) {
    workspace_.Preserve();
    if (cluster_)
      cluster_->PreserveWorkspaces();
    std::cerr << "Preserved E2E workspace: " << workspace_.path() << '\n';
  }
  job_->Terminate();
  ForceTerminate(client_);
  ForceTerminate(server_);
  // The cluster tears its tiers down in reverse dependency order: the edges and
  // file store before the config/historian they depend on.
  if (cluster_)
    cluster_->Terminate();
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
    std::string_view service_name,
    const std::function<void(boost::json::object&)>& configure) {
  auto server_json_value =
      boost::json::parse(ReadFileOrEmpty(GetServerSettingsTemplatePath()));
  auto& server_json = server_json_value.as_object();
  ConfigureSignedLicenseFromEnv(server_json);
  server_json["sessions"] = boost::json::array{
      "tcp;passive;host=0.0.0.0;port=" + std::to_string(remote_port)};
  auto& opcua = server_json["opcua"].is_object()
                    ? server_json["opcua"].as_object()
                    : server_json["opcua"].emplace_object();
  opcua["enabled"] = true;
  opcua["url"] =
      boost::json::array{"opc.tcp://127.0.0.1:" + std::to_string(opcua_port)};
  opcua["trace"] = "none";
  ConfigureTelemetry(server_json, service_name, GetOtlpEndpoint());
  if (configure)
    configure(server_json);
  WriteTextFile(ws / "server.json", boost::json::serialize(server_json_value));
}

void ClientServerE2eTest::PrepareWorkspace() {
  // Against an external target the workspace holds only the client's scratch
  // files — there is no local server to lay out a fixture or settings for.
  if (!UsesExternalServer()) {
    PrepareServerFilesystem(workspace_.path(), iec61850_port_);
    // SingleTier identity; StartCluster() rewrites this same file as the proxy.
    WriteServerJson(workspace_.path(), remote_port_, opcua_port_,
                    "scada-e2e-server");
  }

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
  auto remote_host = std::string{"localhost:"} + std::to_string(remote_port_);
  auto opcua_host = std::string{"127.0.0.1:"} + std::to_string(opcua_port_);
  std::string login_user{user};
  std::string login_password{password};
  if (const auto* target = GetExternalServerTarget()) {
    remote_host = target->remote_host;
    opcua_host = target->opcua_host;
    // The deployment owns its own credentials. Substitute them only for the
    // suite's "correct credentials" login (root with the fixture's empty
    // password) — a test that deliberately passes wrong credentials, or a
    // different fixture user, must keep what it asked for.
    if (user == "root" && password.empty()) {
      login_user = target->user;
      login_password = target->password;
    }
  }
  boost::json::object root{
      {"ServerType", std::string{GetServerType(Protocol())}},
      {"Host:Scada", remote_host},
      {"Host:OpcUa", opcua_host},
      {"User", login_user},
      {"Password", login_password},
      {"AutoLogin", true},
  };
  if (!security_mode.empty())
    root["SecurityMode"] = std::string{security_mode};
  WriteTextFile(settings_file_, boost::json::serialize(root));
}

void ClientServerE2eTest::EnableSimulatedHistory() {
  historize_simulated_item_ = true;
}

void ClientServerE2eTest::StartServer() {
  if (const auto* target = GetExternalServerTarget()) {
    // Nothing to launch — assert instead that the deployment's client-facing
    // endpoint is actually accepting connections, so an unreachable target
    // fails here rather than as an opaque client login timeout.
    const std::string& endpoint = Protocol() == E2eProtocol::OpcUa
                                      ? target->opcua_host
                                      : target->remote_host;
    ASSERT_TRUE(
        WaitUntil([&endpoint] { return CanConnectTcpEndpoint(endpoint); },
                  std::chrono::duration_cast<std::chrono::milliseconds>(
                      kServerStartTimeout)))
        << "External " << ToString(Protocol()) << " endpoint " << endpoint
        << " is not accepting connections";
    return;
  }

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
  // The downstream tiers (config, historian, the three edges, the file store)
  // come from the shared harness — the same one the service x namespace sweep
  // stands up, so the two suites cannot drift on which downstream owns what.
  // The proxy is launched here rather than by the cluster: it reuses the
  // built-in server_ slot on the client-facing ports, and workspace_ doubles as
  // the Qt client's scratch directory.
  cluster_ = std::make_unique<ServerCluster>(
      ClusterExecutables{
          .config = GetConfigExePath(),
          .historian = GetHistorianExePath(),
          .iec104 = GetIec104ExePath(),
          .modbus = GetModbusExePath(),
          .iec61850 = GetIec61850ExePath(),
          .filesystem = GetFilesystemExePath(),
      },
      [](const std::filesystem::path& exe) {
        return MakeServerContextForExe(exe);
      });

  ASSERT_TRUE(cluster_->Start(
      ports_, ClusterOptions{
                  .iec61850_port = iec61850_port_,
                  .proxy_opcua_url =
                      "opc.tcp://127.0.0.1:" + std::to_string(opcua_port_),
                  .historize_simulated_item = historize_simulated_item_,
                  .otlp_endpoint = GetOtlpEndpoint(),
              }));
  config_tier_ = cluster_->Tier(ClusterTier::kConfig);
  filesystem_tier_ = cluster_->Tier(ClusterTier::kFilesystem);

  // --- Proxy (client-facing) -------------------------------------------------
  // Keeps workspace_'s local config DB to authenticate the client's login, with
  // its data-item rows stripped so the aggregated edge namespace stays
  // authoritative for live values.
  ExecuteConfigurationSql(MakeServerContextForExe(GetProxyExePath()),
                          workspace_.path(),
                          std::string{kStripDataItemRowsSql});
  boost::json::array aggregation_servers = cluster_->AggregationServers();
  WriteServerJson(
      workspace_.path(), remote_port_, opcua_port_, "scada-e2e-proxy",
      [&aggregation_servers](boost::json::object& server_json) {
        ConfigureProxyRole(server_json,
                           ProxyRoleOptions{
                               .aggregation_servers = &aggregation_servers,
                               // WriteServerJson already applied the suite's
                               // telemetry settings for this service name.
                               .otlp_endpoint = {},
                           });
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

  // The dynamically-registered modbus edge and the HD-registered historian
  // arrive through discovery after the proxy is up, so tests that browse or
  // read history right after startup would otherwise race them.
  ASSERT_TRUE(WaitForProxyDownstreams(server_log_dir_));
}

void ClientServerE2eTest::StartClient(std::vector<std::string> extra_args) {
  std::vector<std::string> args{
      "--test-settings-file=" + settings_file_.string(),
      "--test-status-file=" + status_file_.string(),
      "--test-log-dir=" + client_log_dir_.string()};
  // Client-side telemetry is metrics only today (the client runs no trace sink
  // and no OTLP log sink — see docs/ops/e2e-client-server.md, "Viewing a run's
  // telemetry"), so this exports "scada-client" meters and nothing else.
  if (const std::string otlp_endpoint = GetOtlpEndpoint();
      !otlp_endpoint.empty()) {
    args.push_back("--otlp-endpoint=" + otlp_endpoint);
    // The client's 1-minute default export period outlasts an E2E case, which
    // would leave "scada-client" absent from the viewer entirely.
    args.push_back("--otlp-export-interval-ms=2000");
  }
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

// The profile lives on the account's UserExtensionType row, keyed by the
// account NAME (the standard user model has no room for a client profile, and
// the extension outlives the UserType folder). These helpers still take the
// UserType id, because that is what a test knows, and join through the name.
namespace {

std::string ProfileColumnForUser(const std::filesystem::path& database,
                                 int user_id,
                                 std::string_view column) {
  return RunSqliteScalar(
      database,
      std::string{"SELECT COALESCE(e."} + std::string{column} +
          ", '') FROM UserExtensionType e JOIN UserType u"
          " ON u.DisplayName = e.DisplayName WHERE u.ID = " +
          std::to_string(user_id) + ";");
}

}  // namespace

std::string ClientServerE2eTest::ReadUserProfileJsonFromServerDatabase(
    int user_id) {
  return ProfileColumnForUser(ServerConfigDatabasePath(), user_id,
                              "ProfileJson");
}

std::string ClientServerE2eTest::ReadUserProfileRevisionFromServerDatabase(
    int user_id) {
  return ProfileColumnForUser(ServerConfigDatabasePath(), user_id,
                              "ProfileRevision");
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
  if (UsesExternalServer()) {
    // The deployment's processes are not ours to observe; what this suite can
    // still assert is that the client survives the window and the endpoint
    // keeps accepting connections.
    auto exited = WaitUntil([this] { return !client_.IsRunning(); }, timeout);
    EXPECT_FALSE(exited) << "Unexpected client exit while " << context << ": "
                         << DescribeProcessExit(client_, "client");
    ExpectServerRemainsRunningFor(std::chrono::milliseconds::zero(), context);
    return;
  }

  auto ok = WaitUntil(
      [this] { return !server_.IsRunning() || !client_.IsRunning(); }, timeout);
  EXPECT_FALSE(ok) << "Unexpected process exit while " << context << ": "
                   << DescribeProcessExit(server_, "server") << ", "
                   << DescribeProcessExit(client_, "client");
}

void ClientServerE2eTest::ExpectServerRemainsRunningFor(
    std::chrono::milliseconds timeout,
    std::string_view context) {
  if (const auto* target = GetExternalServerTarget()) {
    // Liveness of an external deployment is observable only through its
    // endpoint: wait out the window, then require it to still accept a
    // connection (a server that died mid-window refuses or times out).
    const std::string& endpoint = Protocol() == E2eProtocol::OpcUa
                                      ? target->opcua_host
                                      : target->remote_host;
    std::this_thread::sleep_for(timeout);
    EXPECT_TRUE(CanConnectTcpEndpoint(endpoint))
        << "External endpoint " << endpoint << " stopped accepting connections "
        << "while " << context;
    return;
  }

  auto ok = WaitUntil([this] { return !server_.IsRunning(); }, timeout);
  EXPECT_FALSE(ok) << "Unexpected server exit while " << context << ": "
                   << DescribeProcessExit(server_, "server");
}

void ClientServerE2eTest::ExpectServerAuthLog() {
  // The server's logs live on the deployment's host, out of the suite's reach;
  // the client-side login assertions carry the case instead.
  if (UsesExternalServer())
    return;

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
