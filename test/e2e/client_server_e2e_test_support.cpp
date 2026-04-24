#include "test/e2e/client_server_e2e_test_support.h"

#include "base/win/scoped_handle.h"

#include <boost/asio/connect.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/json.hpp>

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <iterator>
#include <memory>
#include <stdexcept>
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

std::wstring QuotePathArg(const std::filesystem::path& arg) {
  return L"\"" + arg.wstring() + L"\"";
}

std::wstring QuoteStringArg(std::wstring_view arg) {
  return L"\"" + std::wstring{arg} + L"\"";
}

void WriteTextFile(const std::filesystem::path& path, std::string_view text) {
  std::error_code ec;
  if (path.has_parent_path())
    std::filesystem::create_directories(path.parent_path(), ec);
  std::ofstream output{path, std::ios::binary | std::ios::trunc};
  if (!output.is_open())
    throw std::runtime_error{"Failed to open " + path.string()};
  output << text;
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
                 DWORD timeout_ms = 5000) {
  if (!process_info.process_handle())
    return;
  WaitForSingleObject(process_info.process_handle(), timeout_ms);
}

void LaunchProcess(const std::filesystem::path& exe,
                   const std::vector<std::wstring>& args,
                   const std::filesystem::path& workdir,
                   JobObject& job,
                   ChildProcess& child);

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

class JobObject {
 public:
  JobObject() {
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

  void Assign(HANDLE process) {
    if (!AssignProcessToJobObject(handle_.Get(), process))
      throw std::runtime_error{"AssignProcessToJobObject failed"};
  }

 private:
  base::win::ScopedHandle handle_;
};

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

std::string ReadFileOrEmpty(const std::filesystem::path& path) {
  std::ifstream input{path, std::ios::binary};
  if (!input)
    return {};
  return {std::istreambuf_iterator<char>{input},
          std::istreambuf_iterator<char>{}};
}

bool ContainsInDirectory(const std::filesystem::path& dir,
                         std::string_view needle) {
  std::error_code ec;
  if (!std::filesystem::exists(dir, ec))
    return false;

  for (const auto& entry : std::filesystem::directory_iterator{dir, ec}) {
    if (!entry.is_regular_file())
      continue;
    if (ReadFileOrEmpty(entry.path()).find(needle) != std::string::npos)
      return true;
  }
  return false;
}

std::optional<int> FindLoggedObjectTreeChildCount(
    const std::filesystem::path& dir) {
  constexpr std::string_view kCompletionMarker =
      "Children fetched callback completed";
  constexpr std::string_view kAddedChildCountMarker = "AddedChildCount = ";

  std::error_code ec;
  if (!std::filesystem::exists(dir, ec))
    return std::nullopt;

  std::optional<int> result;
  for (const auto& entry : std::filesystem::directory_iterator{dir, ec}) {
    if (!entry.is_regular_file())
      continue;

    const auto contents = ReadFileOrEmpty(entry.path());
    size_t pos = 0;
    while ((pos = contents.find(kCompletionMarker.data(), pos)) !=
           std::string::npos) {
      const auto line_end = contents.find('\n', pos);
      const auto line = contents.substr(
          pos,
          line_end == std::string::npos ? std::string::npos : line_end - pos);
      pos = line_end == std::string::npos ? contents.size() : line_end + 1;

      const auto count_pos = line.find(kAddedChildCountMarker);
      if (count_pos == std::string::npos)
        continue;

      const auto value_begin = count_pos + kAddedChildCountMarker.size();
      size_t value_end = value_begin;
      while (value_end < line.size() && line[value_end] >= '0' &&
             line[value_end] <= '9') {
        ++value_end;
      }
      if (value_end == value_begin)
        continue;

      result = std::stoi(line.substr(value_begin, value_end - value_begin));
      if (*result > 0)
        return result;
    }
  }

  return result;
}

TempWorkspace::TempWorkspace() {
  auto base = std::filesystem::temp_directory_path();
  auto salt =
      std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
  path_ = base / ("scada_e2e_" + salt);
  std::filesystem::create_directories(path_);
}

TempWorkspace::~TempWorkspace() {
  if (preserve_)
    return;
  std::error_code ec;
  std::filesystem::remove_all(path_, ec);
}

ClientServerE2eTest::ClientServerE2eTest()
    : job_{std::make_unique<JobObject>()} {}

ClientServerE2eTest::~ClientServerE2eTest() = default;

void ClientServerE2eTest::SetUp() {
  ASSERT_TRUE(std::filesystem::exists(GetServerExePath()));
  ASSERT_TRUE(std::filesystem::exists(GetClientExePath()));
  ASSERT_TRUE(std::filesystem::exists(GetServerFixtureDir()));

  remote_port_ = FindAvailablePort();
  opcua_port_ = FindAvailablePort();
  while (opcua_port_ == remote_port_)
    opcua_port_ = FindAvailablePort();
  PrepareWorkspace();
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
}

void ClientServerE2eTest::PrepareWorkspace() {
  std::filesystem::copy(GetServerFixtureDir(),
                        workspace_.path(),
                        std::filesystem::copy_options::recursive |
                            std::filesystem::copy_options::overwrite_existing);

  boost::json::object server_json{
      {"configuration",
       {{"driver", "SQLite"},
        {"dir", (workspace_.path() / "Configuration").string()}}},
      {"history",
       {{"enabled", false}, {"dir", (workspace_.path() / "History").string()}}},
      {"sessions",
       boost::json::array{
           "tcp;passive;host=0.0.0.0;port=" + std::to_string(remote_port_)}},
      {"opc", {{"client", {{"enabled", false}}}}},
      {"opcua",
       {{"enabled", true},
        {"url", "opc.tcp://127.0.0.1:" + std::to_string(opcua_port_)},
        {"trace", "none"}}},
      {"iec60870", {{"enabled", false}}},
      {"iec61850", {{"enabled", false}}},
      {"modbus", {{"enabled", false}}},
      {"filesystem",
       {{"enabled", true},
        {"dir", (workspace_.path() / "FileSystem").string()}}},
      {"vidicon", {{"enabled", false}}},
      {"log", {{"dir", (workspace_.path() / "ServerLogs").string()}}}};
  WriteTextFile(workspace_.path() / "server.json",
                boost::json::serialize(server_json));

  status_file_ = workspace_.path() / "client-status.txt";
  operator_use_cases_file_ = workspace_.path() / "operator-use-cases.txt";
  settings_file_ = workspace_.path() / "client-settings.json";
  server_log_dir_ = workspace_.path() / "ServerLogs";
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

namespace {

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

}  // namespace
}  // namespace client::test
