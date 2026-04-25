#include "test/e2e/client_server_e2e_test_support.h"

#include "test/e2e/iec61850_test_server.h"

#include <boost/asio/connect.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/json.hpp>

#include <cstdlib>
#include <iostream>
#include <memory>
#include <process.h>
#include <thread>

using namespace std::chrono_literals;

namespace client::test {
namespace {

bool IsKeepWorkspaceEnabled() {
  char* value = nullptr;
  size_t size = 0;
  auto err = _dupenv_s(&value, &size, "SCADA_E2E_KEEP_WORKSPACE");
  std::unique_ptr<char, decltype(&std::free)> holder{value, &std::free};
  return err == 0 && holder && *holder;
}

constexpr auto kWaitStep = 100ms;
constexpr auto kServerStartTimeout = 30s;
constexpr auto kClientStartTimeout = 30s;
constexpr auto kServerLogTimeout = 10s;
constexpr auto kObjectTreeLoadTimeout = 15s;
constexpr auto kObjectViewValuesTimeout = 30s;
constexpr auto kObjectTreeLabelsTimeout = 30s;
constexpr auto kHardwareTreeDevicesTimeout = 30s;
constexpr auto kOperatorUseCasesTimeout = 30s;

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

std::wstring GetSqliteExePath() {
  wchar_t* value = nullptr;
  size_t size = 0;
  auto err = _wdupenv_s(&value, &size, L"SCADA_SQLITE3_EXE");
  std::unique_ptr<wchar_t, decltype(&std::free)> holder{value, &std::free};
  if (err == 0 && holder && *holder)
    return holder.get();
  return std::filesystem::path{SCADA_E2E_SQLITE3_EXE}.wstring();
}

std::string SqlitePath(const std::filesystem::path& path) {
  auto result = path.lexically_normal().generic_string();
  for (auto& ch : result) {
    if (ch == '\'')
      ch = ' ';
  }
  return result;
}

void GenerateConfigurationDatabase(const std::filesystem::path& workspace,
                                   int iec61850_port) {
  const auto configuration_dir = workspace / "Configuration";
  const auto database_path = configuration_dir / "configuration.sqlite3";
  const auto script_path = workspace / "generate-configuration.sql";
  std::error_code ec;
  std::filesystem::remove_all(configuration_dir, ec);
  std::filesystem::create_directories(configuration_dir, ec);

  WriteTextFile(script_path,
                ".bail on\n"
                ".read " +
                    SqlitePath(GetConfigurationBaseSqlPath()) + "\n"
                    ".read " +
                    SqlitePath(GetConfigurationFixtureSqlPath()) + "\n"
                    "UPDATE Iec61850DeviceType SET Port = " +
                    std::to_string(iec61850_port) + ";\n");

  const auto sqlite_exe = GetSqliteExePath();
  std::vector<const wchar_t*> args{
      sqlite_exe.c_str(),
      L"-batch",
      L"-init",
      script_path.c_str(),
      database_path.c_str(),
      nullptr};
  auto exit_code = _wspawnvp(_P_WAIT, sqlite_exe.c_str(), args.data());
  if (exit_code != 0) {
    throw std::runtime_error{"sqlite3 failed while creating " +
                             database_path.string() + " with exit code " +
                             std::to_string(exit_code)};
  }
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

  remote_port_ = FindAvailablePort();
  opcua_port_ = FindAvailablePort();
  while (opcua_port_ == remote_port_)
    opcua_port_ = FindAvailablePort();
  iec61850_port_ = FindAvailablePort();
  while (iec61850_port_ == remote_port_ || iec61850_port_ == opcua_port_)
    iec61850_port_ = FindAvailablePort();
  PrepareWorkspace();

  iec61850_server_ = std::make_unique<Iec61850TestServer>(iec61850_port_);
  ASSERT_TRUE(WaitUntil(
      [this] { return iec61850_server_->running() || iec61850_server_->failed(); },
      5s));
  ASSERT_FALSE(iec61850_server_->failed());
}

void ClientServerE2eTest::TearDown() {
  if (HasFailure() || IsKeepWorkspaceEnabled()) {
    workspace_.Preserve();
    std::cerr << "Preserved E2E workspace: " << workspace_.path() << '\n';
  }
  ForceTerminate(client_.process_info);
  ForceTerminate(server_.process_info);
  WaitForExit(client_.process_info);
  WaitForExit(server_.process_info);
  iec61850_server_.reset();
}

void ClientServerE2eTest::PrepareWorkspace() {
  std::filesystem::copy(GetServerFixtureDir(),
                        workspace_.path(),
                        std::filesystem::copy_options::recursive |
                            std::filesystem::copy_options::overwrite_existing);
  GenerateConfigurationDatabase(workspace_.path(), iec61850_port_);

  auto server_json_value =
      boost::json::parse(ReadFileOrEmpty(GetServerSettingsTemplatePath()));
  auto& server_json = server_json_value.as_object();
  server_json["sessions"] = boost::json::array{
      "tcp;passive;host=0.0.0.0;port=" + std::to_string(remote_port_)};
  auto& opcua = server_json["opcua"].is_object()
                    ? server_json["opcua"].as_object()
                    : server_json["opcua"].emplace_object();
  opcua["enabled"] = true;
  opcua["url"] = boost::json::array{
      "opc.tcp://127.0.0.1:" + std::to_string(opcua_port_)};
  opcua["trace"] = "none";
  WriteTextFile(workspace_.path() / "server.json",
                boost::json::serialize(server_json_value));

  status_file_ = workspace_.path() / "client-status.txt";
  object_view_values_file_ = workspace_.path() / "object-view-values.txt";
  object_tree_labels_file_ = workspace_.path() / "object-tree-labels.txt";
  hardware_tree_devices_file_ =
      workspace_.path() / "hardware-tree-devices.txt";
  operator_use_cases_file_ = workspace_.path() / "operator-use-cases.txt";
  settings_file_ = workspace_.path() / "client-settings.json";
  server_log_dir_ = workspace_.path() / "Logs";
  client_log_dir_ = workspace_.path() / "ClientLogs";
}

void ClientServerE2eTest::WriteClientSettings(std::string_view password) {
  const auto remote_host =
      std::string{"localhost:"} + std::to_string(remote_port_);
  const auto opcua_host =
      std::string{"127.0.0.1:"} + std::to_string(opcua_port_);
  boost::json::object root{
      {"ServerType", std::string{GetServerType(GetParam())}},
      {"Host:Scada", remote_host},
      {"Host:OpcUa", opcua_host},
      {"User", "root"},
      {"Password", std::string{password}},
      {"AutoLogin", true},
  };
  WriteTextFile(settings_file_, boost::json::serialize(root));
}

void ClientServerE2eTest::StartServer() {
  LaunchProcess(GetServerExePath(),
                {L"--param=" + (workspace_.path() / "server.json").wstring()},
                workspace_.path(),
                *job_,
                server_);

  const int port = GetProtocolPort();
  ASSERT_TRUE(WaitUntil([port] { return CanConnectTcp(port); },
                        std::chrono::duration_cast<std::chrono::milliseconds>(
                            kServerStartTimeout)))
      << "Server did not start listening on " << ToString(GetParam()) << " port "
      << port;
}

void ClientServerE2eTest::StartClient(std::vector<std::wstring> extra_args) {
  std::vector<std::wstring> args{
      L"--test-settings-file=" + settings_file_.wstring(),
      L"--test-status-file=" + status_file_.wstring(),
      L"--test-log-dir=" + client_log_dir_.wstring()};
  args.insert(args.end(),
              std::make_move_iterator(extra_args.begin()),
              std::make_move_iterator(extra_args.end()));

  LaunchProcess(GetClientExePath(),
                args,
                GetClientExePath().parent_path(),
                *job_,
                client_);
}

std::string ClientServerE2eTest::WaitForStatus() {
  bool ok = WaitUntil(
      [this] { return std::filesystem::exists(status_file_) || !client_.IsRunning(); },
      std::chrono::duration_cast<std::chrono::milliseconds>(kClientStartTimeout));
  EXPECT_TRUE(ok) << "Timed out waiting for client status file";
  return ReadFileOrEmpty(status_file_);
}

bool ClientServerE2eTest::WaitForStartupOrStatus() {
  return WaitUntil(
      [this] {
        return ContainsInDirectory(client_log_dir_, kStartupCompletedLog) ||
               std::filesystem::exists(status_file_) || !client_.IsRunning();
      },
      std::chrono::duration_cast<std::chrono::milliseconds>(kClientStartTimeout));
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

std::string ClientServerE2eTest::DescribeProcessExit(
    const ChildProcess& process,
    std::string_view name) const {
  auto exit_code = process.ExitCode();
  if (!exit_code)
    return std::string{name} + " process exit code unavailable";
  if (*exit_code == STILL_ACTIVE)
    return std::string{name} + " is still running";
  return std::string{name} + " exited with code " +
         std::to_string(static_cast<unsigned long>(*exit_code));
}

void ClientServerE2eTest::ExpectProcessesRemainRunningFor(
    std::chrono::milliseconds timeout,
    std::string_view context) {
  auto ok = WaitUntil([this] { return !server_.IsRunning() || !client_.IsRunning(); },
                      timeout);
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
  EXPECT_TRUE(WaitUntil([this] {
    return ContainsInDirectory(server_log_dir_, "Authorization succeeded") ||
           ContainsInDirectory(server_log_dir_, "CreateSession completed");
  },
                        std::chrono::duration_cast<std::chrono::milliseconds>(
                            kServerLogTimeout)))
      << "Server logs did not record a successful session in "
      << server_log_dir_;
}

int ClientServerE2eTest::GetProtocolPort() const {
  switch (GetParam()) {
    case E2eProtocol::Remote:
      return remote_port_;
    case E2eProtocol::OpcUa:
      return opcua_port_;
  }
  return 0;
}

}  // namespace client::test
