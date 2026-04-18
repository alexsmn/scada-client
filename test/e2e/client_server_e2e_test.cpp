#include "base/utf_convert.h"
#include "base/win/scoped_handle.h"
#include "base/win/scoped_process_information.h"

#include <boost/asio/connect.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/json.hpp>
#include <gtest/gtest.h>
#include <Windows.h>

#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <random>
#include <string>
#include <stdexcept>
#include <thread>
#include <vector>

using namespace std::chrono_literals;

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
constexpr auto kPostConnectStabilityTimeout = 10s;
constexpr auto kObjectTreeLoadTimeout = 15s;
constexpr std::string_view kStartupCompletedLog =
    "Client startup completed; entering steady-state run loop";

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

std::string ReadFileOrEmpty(const std::filesystem::path& path) {
  std::ifstream input{path, std::ios::binary};
  if (!input)
    return {};
  return {std::istreambuf_iterator<char>{input}, std::istreambuf_iterator<char>{}};
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
  constexpr std::string_view kNodeIdMarker = "NodeId = SCADA.24";
  constexpr std::string_view kAddedChildCountMarker = "AddedChildCount = ";

  std::error_code ec;
  if (!std::filesystem::exists(dir, ec))
    return std::nullopt;

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

      if (line.find(kNodeIdMarker) == std::string::npos)
        continue;

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

      return std::stoi(line.substr(value_begin, value_end - value_begin));
    }
  }

  return std::nullopt;
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

struct ChildProcess {
  base::win::ScopedProcessInformation process_info;

  bool IsRunning() const {
    if (!process_info.process_handle())
      return false;
    DWORD exit_code = 0;
    return GetExitCodeProcess(process_info.process_handle(), &exit_code) &&
           exit_code == STILL_ACTIVE;
  }

  std::optional<DWORD> ExitCode() const {
    if (!process_info.process_handle())
      return std::nullopt;

    DWORD exit_code = 0;
    if (!GetExitCodeProcess(process_info.process_handle(), &exit_code))
      return std::nullopt;
    return exit_code;
  }
};

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

class TempWorkspace {
 public:
  TempWorkspace() {
    auto base = std::filesystem::temp_directory_path();
    auto salt =
        std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
    path_ = base / ("scada_e2e_" + salt);
    std::filesystem::create_directories(path_);
  }

  ~TempWorkspace() {
    if (preserve_)
      return;
    std::error_code ec;
    std::filesystem::remove_all(path_, ec);
  }

  const std::filesystem::path& path() const { return path_; }

  void Preserve() { preserve_ = true; }

 private:
  std::filesystem::path path_;
  bool preserve_ = false;
};

class ClientServerE2eTest : public ::testing::Test {
 protected:
  void SetUp() override {
    ASSERT_TRUE(std::filesystem::exists(GetServerExePath()));
    ASSERT_TRUE(std::filesystem::exists(GetClientExePath()));
    ASSERT_TRUE(std::filesystem::exists(GetServerFixtureDir()));

    port_ = FindAvailablePort();
    PrepareWorkspace();
  }

  void TearDown() override {
    if (HasFailure() || IsKeepWorkspaceEnabled()) {
      workspace_.Preserve();
      std::cerr << "Preserved E2E workspace: " << workspace_.path() << '\n';
    }
    ForceTerminate(client_.process_info);
    ForceTerminate(server_.process_info);
    WaitForExit(client_.process_info);
    WaitForExit(server_.process_info);
  }

  void PrepareWorkspace() {
    std::filesystem::copy(GetServerFixtureDir(),
                          workspace_.path(),
                          std::filesystem::copy_options::recursive |
                              std::filesystem::copy_options::overwrite_existing);

    boost::json::object server_json{
        {"configuration",
         {{"driver", "SQLite"},
          {"dir", (workspace_.path() / "Configuration").string()}}},
        {"history", {{"dir", (workspace_.path() / "History").string()}}},
        {"sessions",
         boost::json::array{
             "tcp;passive;host=0.0.0.0;port=" + std::to_string(port_)}},
        {"opc", {{"client", {{"enabled", false}}}}},
        {"opcua",
         {{"enabled", false},
          {"url", "opc.tcp://localhost:4840"},
          {"trace", "none"}}},
        {"filesystem",
         {{"enabled", true},
          {"dir", (workspace_.path() / "FileSystem").string()}}},
        {"vidicon", {{"enabled", false}}},
        {"log", {{"dir", (workspace_.path() / "ServerLogs").string()}}}};
    WriteTextFile(workspace_.path() / "server.json",
                  boost::json::serialize(server_json));

    status_file_ = workspace_.path() / "client-status.txt";
    settings_file_ = workspace_.path() / "client-settings.json";
    server_log_dir_ = workspace_.path() / "ServerLogs";
    client_log_dir_ = workspace_.path() / "ClientLogs";
  }

  void WriteClientSettings(std::string_view password) {
    boost::json::object root{
        {"ServerType", "Scada"},
        {"Host:Scada", std::string{"localhost:"} + std::to_string(port_)},
        {"User", "root"},
        {"Password", std::string{password}},
        {"AutoLogin", true},
    };
    WriteTextFile(settings_file_, boost::json::serialize(root));
  }

  void StartServer() {
    LaunchProcess(GetServerExePath(),
                  {L"--param=" + (workspace_.path() / "server.json").wstring()},
                  workspace_.path(),
                  job_,
                  server_);

    ASSERT_TRUE(WaitUntil([this] { return CanConnectTcp(port_); },
                          std::chrono::duration_cast<std::chrono::milliseconds>(
                              kServerStartTimeout)))
        << "Server did not start listening on port " << port_;
  }

  void StartClient() {
    LaunchProcess(
        GetClientExePath(),
        {L"--test-settings-file=" + settings_file_.wstring(),
         L"--test-status-file=" + status_file_.wstring(),
         L"--test-log-dir=" + client_log_dir_.wstring()},
        GetClientExePath().parent_path(),
        job_,
        client_);
  }

  std::string WaitForStatus() {
    bool ok = WaitUntil([this] {
      return std::filesystem::exists(status_file_) ||
             !client_.IsRunning();
    },
                        std::chrono::duration_cast<std::chrono::milliseconds>(
                            kClientStartTimeout));
    EXPECT_TRUE(ok) << "Timed out waiting for client status file";
    return ReadFileOrEmpty(status_file_);
  }

  bool WaitForStartupOrStatus() {
    return WaitUntil(
        [this] {
          return ContainsInDirectory(client_log_dir_, kStartupCompletedLog) ||
                 std::filesystem::exists(status_file_) || !client_.IsRunning();
        },
        std::chrono::duration_cast<std::chrono::milliseconds>(
            kClientStartTimeout));
  }

  bool WaitForObjectTreeReady() {
    return WaitUntil(
        [this] {
          auto child_count = FindLoggedObjectTreeChildCount(client_log_dir_);
          return (child_count && *child_count > 0) || !client_.IsRunning();
        },
        std::chrono::duration_cast<std::chrono::milliseconds>(
            kObjectTreeLoadTimeout));
  }

  std::string DescribeProcessExit(const ChildProcess& process,
                                  std::string_view name) const {
    auto exit_code = process.ExitCode();
    if (!exit_code)
      return std::string{name} + " process exit code unavailable";
    if (*exit_code == STILL_ACTIVE)
      return std::string{name} + " is still running";
    return std::string{name} + " exited with code " +
           std::to_string(static_cast<unsigned long>(*exit_code));
  }

  void ExpectProcessesRemainRunningFor(
      std::chrono::milliseconds timeout,
      std::string_view context) {
    auto ok = WaitUntil([this] {
      return !server_.IsRunning() || !client_.IsRunning();
    }, timeout);
    EXPECT_FALSE(ok)
        << "Unexpected process exit while " << context << ": "
        << DescribeProcessExit(server_, "server") << ", "
        << DescribeProcessExit(client_, "client");
  }

  void ExpectServerRemainsRunningFor(std::chrono::milliseconds timeout,
                                     std::string_view context) {
    auto ok = WaitUntil([this] { return !server_.IsRunning(); }, timeout);
    EXPECT_FALSE(ok) << "Unexpected server exit while " << context << ": "
                     << DescribeProcessExit(server_, "server");
  }

  void ExpectServerAuthLog() {
    EXPECT_TRUE(WaitUntil([this] {
      return ContainsInDirectory(server_log_dir_, "Authorization succeeded") ||
             ContainsInDirectory(server_log_dir_, "CreateSession completed");
    },
                            std::chrono::duration_cast<std::chrono::milliseconds>(
                                kServerLogTimeout)))
        << "Server logs did not record a successful session in "
        << server_log_dir_;
  }

  TempWorkspace workspace_;
  JobObject job_;
  int port_ = 0;

  std::filesystem::path status_file_;
  std::filesystem::path settings_file_;
  std::filesystem::path server_log_dir_;
  std::filesystem::path client_log_dir_;

  ChildProcess server_;
  ChildProcess client_;
};

TEST_F(ClientServerE2eTest, Connect_Success) {
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

TEST_F(ClientServerE2eTest, Connect_Success_LoadsObjectTree) {
  WriteClientSettings(/*password=*/"");
  StartServer();
  StartClient();

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
         "completed first-level object tree fetch for SCADA.24";
  EXPECT_GT(*child_count, 0)
      << "Client loaded the object tree root but did not materialize any "
         "first-level children";

  ExpectServerAuthLog();
  ExpectProcessesRemainRunningFor(
      std::chrono::duration_cast<std::chrono::milliseconds>(
          kPostConnectStabilityTimeout),
      "waiting for the object tree to remain available after login");
}

TEST_F(ClientServerE2eTest, Connect_BadPassword) {
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

}  // namespace
