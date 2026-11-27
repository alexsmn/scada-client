// A display bound in the Designer against a third-party OPC UA server, opened
// by the client against that server and against a scada-proxy aggregating it
// (docs/product/foreign-node-references.md, "What ends this plan").
//
// Three products meet here, and none of them as source. Each one is named by
// path, the way this directory already names tier binaries and the IEC 61850
// device (ADR 0011, backlog 801):
//
//   SCADA_E2E_FOREIGN_SERVER_EXE   open62541, built by the superproject
//                                  (target scada_e2e_open62541_server)
//   SCADA_E2E_DESIGNER_PROBE_EXE   the Designer's designer_server_probe, whose
//                                  --bind writes a display through the
//                                  Designer's own binding and save code
//   SCADA_E2E_PROXY_EXE            scada-proxy
//   SCADA_DISPLAY_RUNTIME_LIBRARY  the display runtime the client loads
//
// Two cases. The first runs the client half in this process: the OpcUa
// backend's services (CreateRemappingClientDataServices, as
// client/modules/opcua_services/opcua_services_module.cpp builds them), the
// node service and timed-data service ClientApplication builds over them, the
// runtime-backed DisplayWidget a Vidicon display view opens, and the
// DisplayLiveData that feeds it, wired as VidiconDisplayNativeView wires them.
//
// The second runs the client itself, `client_qt` (SCADA_E2E_CLIENT_EXE),
// offscreen, signed in to open62541 through its own login: the display is
// copied into the public folder before sign-in (decision B), and the client's
// --test-display-menu-file hook (client/app/qt/e2e_test_support.cpp) opens the
// main window's Display menu, chooses the file's row and reports what the
// schematic it opened was given.

#include "test/e2e/e2e_cluster.h"
#include "test/e2e/e2e_port_reservation.h"
#include "test/e2e/e2e_process.h"
#include "test/e2e/e2e_server_process.h"
#include "test/e2e/e2e_wait.h"

#include "aui/test/app_environment.h"
#include "base/awaitable.h"
#include "common/test/scoped_temp_dir.h"
#include "events/node_event_provider_mock.h"
#include "model/namespace_uris.h"
#include "model/node_id_util.h"
#include "modules/display_view/display_live_data.h"
#include "modules/display_view/qt/display_widget.h"
#include "node_service/node_service.h"
#include "node_service/node_service_factory.h"
#include "opcua/client/client_session.h"
#include "opcua_bridge/client_adapters.h"
#include "scada/attribute_service.h"
#include "scada/client.h"
#include "scada/data_services.h"
#include "scada/session_service.h"
#include "scada/status.h"
#include "services/alias_resolver_factory.h"
#include "timed_data/timed_data_context.h"
#include "timed_data/timed_data_service.h"
#include "timed_data/timed_data_service_factory.h"
#include "transport/transport_factory_impl.h"

#include <boost/asio/io_context.hpp>
#include <boost/json.hpp>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <memory>
#include <optional>
#include <regex>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace client::test {
namespace {

using namespace std::chrono_literals;

// The object the engineer drags onto the display: the fixture's writable
// Plant.Setpoint, in a namespace the client's model does not know
// (test/e2e/foreign_server/open62541_test_server.h in the superproject).
constexpr std::string_view kBoundObjectPath = "Plant/Setpoint";
constexpr std::string_view kForeignNamespaceUri =
    "urn:telecontrol:test:foreign-server";

// What the test writes, so the value the display shows can only have come
// from this run's server.
constexpr double kLiveValue = 73.25;

// The name the display is copied into the public folder under, and so the
// Display menu's row for it: FileCache lists a file by its stem.
constexpr std::string_view kPublicFileStem = "Foreign plant";

// What a build was configured without, as the -D flags that would supply it.
std::string MissingPaths() {
  std::string missing;
  const auto need = [&](std::string_view value, std::string_view flag) {
    if (value.empty())
      missing += std::string{"\n  -D"} + std::string{flag} + "=<path>";
  };
  need(SCADA_E2E_FOREIGN_SERVER_EXE, "SCADA_E2E_FOREIGN_SERVER_EXE");
  need(SCADA_E2E_DESIGNER_PROBE_EXE, "SCADA_E2E_DESIGNER_PROBE_EXE");
  need(SCADA_E2E_FOREIGN_DISPLAY_PROXY_EXE, "SCADA_E2E_PROXY_EXE");
  need(SCADA_DISPLAY_RUNTIME_UNDER_TEST, "SCADA_DISPLAY_RUNTIME_LIBRARY");
  return missing;
}

// The licence the proxy runs under, as for every tier this suite launches.
std::filesystem::path LicensePath() {
  const char* env = std::getenv("SCADA_SERVER_LICENSE_FILE");
  return env && *env ? std::filesystem::path{env} : std::filesystem::path{};
}

// The open62541 fixture as a child process: `--port=<n>`, ready once the port
// accepts a connection.
class ForeignServer {
 public:
  ForeignServer(const std::filesystem::path& exe,
                int port,
                const std::filesystem::path& workdir)
      : port_{port} {
    LaunchProcess(exe, {"--port=" + std::to_string(port)}, workdir, job_,
                  child_, {}, workdir);
    WaitUntil([this] { return CanConnectTcp(port_) || !child_.IsRunning(); },
              Timeout(10s));
    started_ = CanConnectTcp(port_) && child_.IsRunning();
  }

  ~ForeignServer() {
    if (child_.IsRunning())
      ForceTerminate(child_);
    job_.Terminate();
  }

  ForeignServer(const ForeignServer&) = delete;
  ForeignServer& operator=(const ForeignServer&) = delete;

  bool started() const { return started_; }
  std::string endpoint_url() const {
    return "opc.tcp://127.0.0.1:" + std::to_string(port_);
  }

 private:
  const int port_;
  JobObject job_;
  ChildProcess child_;
  bool started_ = false;
};

// Runs the Designer's probe to bind `object_path` on the server at `url` into
// the first shape of `template_vds`, saving `out_vds`. Returns the probe's
// output; `exit_code` receives its exit status.
std::string RunDesignerBind(const std::string& url,
                            std::string_view object_path,
                            const std::filesystem::path& template_vds,
                            const std::filesystem::path& out_vds,
                            const std::filesystem::path& workdir,
                            int& exit_code) {
  JobObject job;
  ChildProcess child;
  // The fixture serves Security None to anonymous sessions only, which the
  // Designer's encrypted Connect cannot use; the probe's unsecured anonymous
  // session is the same ServerSession from the moment it is open, and the
  // binding is all this check is about.
  LaunchProcess(std::filesystem::path{SCADA_E2E_DESIGNER_PROBE_EXE},
                {url, "-", "-", "--unsecured-anonymous", "--pki",
                 (workdir / "designer-pki").string(), "--depth", "0", "--bind",
                 std::string{object_path}, "--into", template_vds.string(),
                 "--save", out_vds.string()},
                workdir, job, child, {}, workdir);
  WaitForExit(child, 30000);
  exit_code = child.ExitCode().value_or(-1);
  std::ifstream out{workdir / "process.stdout.log"};
  std::ifstream err{workdir / "process.stderr.log"};
  std::stringstream text;
  text << out.rdbuf() << err.rdbuf();
  return text.str();
}

// The client's OpcUa backend and the services ClientApplication builds over
// it, on one io_context the test drives.
class ClientConnection {
 public:
  ClientConnection()
      : transport_factory_{transport::CreateTransportFactory()},
        session_{std::make_shared<opcua::ClientSession>(io_.get_executor(),
                                                        *transport_factory_)},
        services_{scada::opcua_bridge::CreateRemappingClientDataServices(
            session_,
            scada::model::GetCanonicalNamespaceUris())} {}

  ~ClientConnection() {
    timed_data_service_.reset();
    node_service_.reset();
    if (connected_) {
      RunAwaitable(io_,
                   [this] { return services_.session_service_->Disconnect(); });
    }
  }

  ClientConnection(const ClientConnection&) = delete;
  ClientConnection& operator=(const ClientConnection&) = delete;

  scada::Status Connect(const std::string& url, std::u16string user = {}) {
    scada::Status status{scada::StatusCode::Bad};
    RunAwaitable(io_, [&]() -> Awaitable<void> {
      status = co_await services_.session_service_->ConnectStatus(
          scada::SessionConnectParams{.connection_string = url,
                                      .user_name = user});
    });
    connected_ = static_cast<bool>(status);
    if (!connected_)
      return status;

    const AnyExecutor executor{io_.get_executor()};
    // As MasterDataServices::as_services hands them on: the executor the
    // client-side monitored item drives its subscription on.
    scada::services services = services_.as_services();
    services.monitored_item_executor = executor;
    node_service_ = CreateNodeService(NodeServiceContext{
        .executor_ = executor,
        .session_service_ = *services_.session_service_,
        .attribute_service_ = *services_.attribute_service_,
        .view_service_ = *services_.view_service_,
        .monitored_item_service_ = *services_.monitored_item_service_,
        .method_service_ = *services_.method_service_,
        .scada_client_ = scada::client{services}});
    timed_data_service_ = CreateTimedDataService(CoroutineTimedDataContext{
        .executor_ = executor,
        .alias_resolver_ = CreateAliasResolver(*node_service_),
        .node_service_ = *node_service_,
        .history_service_ = services_.history_service_,
        .node_event_provider_ = node_event_provider_,
        .namespace_index_for_uri_ = services_.namespace_index_for_uri_});
    return status;
  }

  // The node a stored reference names, through this connection.
  scada::NodeId Resolve(std::string_view reference) {
    return scada::NodeIdFromStoredString(reference,
                                         services_.namespace_index_for_uri_);
  }

  scada::Status Write(const scada::NodeId& node_id, scada::Variant value) {
    scada::Status status{scada::StatusCode::Bad};
    RunAwaitable(io_, [&]() -> Awaitable<void> {
      status = co_await scada::Write(
          *services_.attribute_service_, scada::ServiceContext{},
          scada::WriteValue{.node_id = node_id, .value = std::move(value)});
    });
    return status;
  }

  TimedDataService& timed_data_service() { return *timed_data_service_; }

  // Runs the client's executor until `done` holds or `timeout` passes.
  template <class Done>
  bool RunUntil(Done done, std::chrono::milliseconds timeout) {
    const auto deadline = std::chrono::steady_clock::now() + timeout;
    while (!done()) {
      if (std::chrono::steady_clock::now() >= deadline)
        return false;
      io_.restart();
      io_.run_for(20ms);
    }
    return true;
  }

 private:
  boost::asio::io_context io_;
  std::shared_ptr<transport::TransportFactory> transport_factory_;
  std::shared_ptr<opcua::ClientSession> session_;
  DataServices services_;
  bool connected_ = false;
  // The alarm side is not what this check is about; events through a proxy
  // and from a foreign server have suites of their own.
  testing::NiceMock<MockNodeEventProvider> node_event_provider_;
  std::shared_ptr<NodeService> node_service_;
  std::unique_ptr<TimedDataService> timed_data_service_;
};

// What a Vidicon display view shows for one binding.
struct Shown {
  std::vector<std::string> data_sources;
  bool shape_found = false;
  std::optional<DisplayDataSourceReading> reading;
};

// Opens `vds` as a Vidicon display view does and waits for a good reading of
// `binding` through `connection`.
Shown OpenDisplay(ClientConnection& connection,
                  const std::filesystem::path& vds,
                  const std::string& binding) {
  Shown shown;
  DisplayWidget widget;
  if (!widget.Open(vds, DisplayDocumentKind::kVds)) {
    ADD_FAILURE() << "the client could not open " << vds << ": "
                  << widget.error_message().toStdString();
    return shown;
  }
  shown.data_sources = widget.DataSources();
  shown.shape_found = widget.SelectDataSourceElement(binding);

  std::optional<DisplayDataSourceReading> latest;
  DisplayLiveData live_data{connection.timed_data_service(), shown.data_sources,
                            [&](const DisplayDataSourceReading& reading) {
                              widget.ShowReading(reading);
                              if (reading.data_source == binding)
                                latest = reading;
                            }};
  connection.RunUntil(
      [&] { return latest && latest->value && latest->quality == 0; },
      Timeout(30s));
  shown.reading = latest;
  return shown;
}

std::string Describe(const std::optional<DisplayDataSourceReading>& reading) {
  if (!reading)
    return "no reading";
  std::ostringstream text;
  text << "text '" << reading->text << "', quality 0x" << std::hex
       << reading->quality << ", value "
       << (reading->value ? std::to_string(*reading->value) : "none");
  return text.str();
}

// The whole of `path`, or empty when it cannot be read.
std::string ReadTextFile(const std::filesystem::path& path) {
  std::ifstream input{path, std::ios::binary};
  std::stringstream text;
  text << input.rdbuf();
  return text.str();
}

// The values of every `<key>=` line of the client's Display menu report.
std::vector<std::string> ReportValues(std::string_view report,
                                      std::string_view key) {
  std::vector<std::string> values;
  const std::string prefix = std::string{key} + "=";
  std::istringstream lines{std::string{report}};
  for (std::string line; std::getline(lines, line);) {
    if (line.starts_with(prefix))
      values.push_back(line.substr(prefix.size()));
  }
  return values;
}

// A `reading=` line: the data source, the text, the quality, the value.
struct ReportedReading {
  std::string data_source;
  std::string text;
  int32_t quality = -1;
  std::optional<double> value;
};

std::optional<ReportedReading> ParseReportedReading(std::string_view line) {
  std::vector<std::string> fields;
  std::istringstream input{std::string{line}};
  for (std::string field; std::getline(input, field, '\t');)
    fields.push_back(field);
  if (fields.size() != 4)
    return std::nullopt;
  ReportedReading reading{.data_source = fields[0], .text = fields[1]};
  try {
    reading.quality = std::stoi(fields[2]);
    if (!fields[3].empty())
      reading.value = std::stod(fields[3]);
  } catch (const std::exception&) {
    return std::nullopt;
  }
  return reading;
}

// The client_qt process, signed in to `opcua_host` anonymously with no
// security (all the fixture serves), its data directory -- and so its public
// folder -- at `data_dir`.
class GuiClient {
 public:
  GuiClient(const std::filesystem::path& workdir,
            const std::filesystem::path& data_dir,
            const std::string& opcua_host)
      : workdir_{workdir},
        report_file_{workdir / "display-menu.txt"},
        status_file_{workdir / "client-status.txt"} {
    std::filesystem::create_directories(workdir_ / "ClientProcess");
    const boost::json::object settings{
        {"ServerType", "OpcUa"},
        {"Host:OpcUa", opcua_host},
        {"User", ""},
        {"Password", ""},
        {"AutoLogin", true},
        {"SecurityMode", "None"},
        {"OpcUaPkiDirectory", (workdir_ / "ClientPki").string()},
    };
    std::ofstream{workdir_ / "client-settings.json"}
        << boost::json::serialize(settings);

    const std::filesystem::path exe{SCADA_E2E_CLIENT_EXE};
    // SCADA_DISPLAY_RUNTIME, set by the fixture, is inherited.
    LaunchProcess(
        exe,
        {"--test-settings-file=" + (workdir_ / "client-settings.json").string(),
         "--test-status-file=" + status_file_.string(),
         "--test-log-dir=" + (workdir_ / "ClientLogs").string(),
         "--test-data-dir=" + data_dir.string(),
         "--test-display-menu-file=" + report_file_.string()},
        exe.parent_path(), job_, child_, {{"QT_QPA_PLATFORM", "offscreen"}},
        workdir_ / "ClientProcess");
  }

  ~GuiClient() {
    if (child_.IsRunning())
      ForceTerminate(child_);
    job_.Terminate();
  }

  GuiClient(const GuiClient&) = delete;
  GuiClient& operator=(const GuiClient&) = delete;

  // Waits for the Display menu report; empty if the client wrote none.
  std::string WaitForReport(std::chrono::milliseconds timeout) {
    WaitUntil(
        [this] {
          return std::filesystem::exists(report_file_) || !child_.IsRunning();
        },
        timeout);
    return ReadTextFile(report_file_);
  }

  std::string status() const { return ReadTextFile(status_file_); }

  // The last `max_chars` of everything the client logged and printed.
  std::string LogTail(size_t max_chars) const {
    std::string text;
    for (const std::filesystem::path dir :
         {workdir_ / "ClientLogs", workdir_ / "ClientProcess"}) {
      std::error_code ec;
      for (const auto& entry : std::filesystem::directory_iterator{dir, ec}) {
        if (entry.is_regular_file())
          text += "== " + entry.path().filename().string() + "\n" +
                  ReadTextFile(entry.path());
      }
    }
    return text.size() > max_chars ? text.substr(text.size() - max_chars)
                                   : text;
  }

 private:
  const std::filesystem::path workdir_;
  const std::filesystem::path report_file_;
  const std::filesystem::path status_file_;
  JobObject job_;
  ChildProcess child_;
};

class ForeignDisplayE2eTest : public ::testing::Test {
 protected:
  void SetUp() override {
    if (const std::string missing = MissingPaths(); !missing.empty()) {
      GTEST_SKIP() << "this build cannot run the foreign display check; "
                      "re-configure naming:"
                   << missing;
    }
    // The library the configure named, before the first display opens:
    // DisplayRuntime loads once per process.
#ifdef _WIN32
    _putenv_s("SCADA_DISPLAY_RUNTIME", SCADA_DISPLAY_RUNTIME_UNDER_TEST);
#else
    ::setenv("SCADA_DISPLAY_RUNTIME", SCADA_DISPLAY_RUNTIME_UNDER_TEST,
             /*overwrite=*/1);
#endif
    workdir_ = temp_dir_.path();
    std::filesystem::create_directories(workdir_ / "server");
    std::filesystem::create_directories(workdir_ / "designer");
  }

  // Steps 1 and 2 of both cases: the Designer binds Plant.Setpoint on
  // `server` into the first shape of the client's fixture display and saves
  // it as `vds`, and the binding it stored is checked to name the node by its
  // namespace URI; then a write moves the value to kLiveValue. Leaves the
  // braced binding in `binding`.
  void BindAndMoveTheValue(const ForeignServer& server,
                           const std::filesystem::path& vds,
                           std::string& binding) {
    int probe_exit = -1;
    const std::string probe_output =
        RunDesignerBind(server.endpoint_url(), kBoundObjectPath,
                        std::filesystem::path{SCADA_E2E_DISPLAY_TEMPLATE}, vds,
                        workdir_ / "designer", probe_exit);
    ASSERT_EQ(probe_exit, 0) << probe_output;
    ASSERT_TRUE(std::filesystem::exists(vds)) << probe_output;

    // The reference the Designer stored names the node by its namespace URI,
    // never by an index one session handed out.
    std::smatch bound;
    ASSERT_TRUE(
        std::regex_search(probe_output, bound, std::regex{R"(bound (\S+))"}))
        << probe_output;
    binding = bound[1];
    EXPECT_NE(binding.find(std::string{"nsu="} +
                           std::string{kForeignNamespaceUri} + ";"),
              std::string::npos)
        << binding;
    EXPECT_FALSE(std::regex_search(binding, std::regex{R"(\bNS\d+\.)"}))
        << binding;
    EXPECT_EQ(binding.find("ns="), std::string::npos) << binding;

    // Something on the plant moves the value.
    ClientConnection writer;
    ASSERT_TRUE(writer.Connect(server.endpoint_url()));
    const scada::NodeId node =
        writer.Resolve(std::string_view{binding}.substr(1, binding.size() - 2));
    ASSERT_FALSE(node.is_null()) << binding;
    ASSERT_TRUE(writer.Write(node, scada::Variant{kLiveValue}));
  }

  AppEnvironment app_env_;
  scada::ScopedTempDir temp_dir_{"client_foreign_display_e2e"};
  PortPool ports_;
  std::filesystem::path workdir_;
};

// The plan's end-to-end check. The Designer binds Plant.Setpoint on open62541
// into a display and saves it; the client opens that file against open62541
// and against a scada-proxy aggregating it, and shows the value this test
// wrote both times.
TEST_F(ForeignDisplayE2eTest,
       ADesignerBoundDisplayShowsTheSameValueDirectAndThroughTheProxy) {
  ForeignServer server{std::filesystem::path{SCADA_E2E_FOREIGN_SERVER_EXE},
                       ports_.Allocate(), workdir_ / "server"};
  ASSERT_TRUE(server.started())
      << "open62541 did not start; see " << workdir_ / "server";

  ASSERT_TRUE(std::filesystem::exists(LicensePath()))
      << "SCADA_SERVER_LICENSE_FILE must name the signed licence the "
         "proxy runs under";

  // 1, 2. The Designer binds the object and saves the display; the value
  // moves.
  const std::filesystem::path vds = workdir_ / "foreign.vds";
  std::string binding;
  BindAndMoveTheValue(server, vds, binding);
  ASSERT_FALSE(HasFatalFailure());

  // 3. The client opens the display against open62541.
  Shown direct;
  {
    ClientConnection client;
    const scada::Status connected = client.Connect(server.endpoint_url());
    ASSERT_TRUE(connected) << ::ToString(connected);
    direct = OpenDisplay(client, vds, binding);
  }
  EXPECT_THAT(direct.data_sources, testing::Contains(binding));
  EXPECT_TRUE(direct.shape_found)
      << "no shape in the opened display is bound to " << binding;
  ASSERT_TRUE(direct.reading && direct.reading->value)
      << "direct: " << Describe(direct.reading);
  EXPECT_DOUBLE_EQ(*direct.reading->value, kLiveValue)
      << "direct: " << Describe(direct.reading);

  // 4. And against a scada-proxy aggregating it.
  ServerTier proxy{ServerProcessContext{
      .server_exe = std::filesystem::path{SCADA_E2E_FOREIGN_DISPLAY_PROXY_EXE},
      .fixture_dir = SCADA_E2E_SERVER_FIXTURE_DIR,
      .settings_template = SCADA_E2E_SERVER_SETTINGS_TEMPLATE,
      .configuration_base_sql = SCADA_E2E_CONFIGURATION_BASE_SQL,
      .configuration_fixture_sql = SCADA_E2E_CONFIGURATION_FIXTURE_SQL,
      .sqlite_exe =
          std::filesystem::path{SCADA_E2E_FOREIGN_DISPLAY_SQLITE3_EXE},
      .configure_license =
          [](boost::json::object& server_json, const std::filesystem::path&) {
            ConfigureSignedLicenseFromEnv(server_json);
          },
  }};
  proxy.AllocatePorts(ports_);
  // What a deployment writes for a foreign downstream: an endpoint, nothing
  // else.
  boost::json::array servers{
      boost::json::object{{"endpoint", server.endpoint_url()}}};
  proxy.Launch(ServerTier::Options{
      .configure =
          [&servers](boost::json::object& json) {
            ConfigureProxyRole(json,
                               ProxyRoleOptions{
                                   .aggregation_servers = &servers,
                                   .service_name = "scada-e2e-foreign-display",
                               });
          },
      .extra_config_sql = std::string{kStripDataItemRowsSql},
  });
  ASSERT_TRUE(proxy.WaitListening())
      << "the proxy did not listen; see " << proxy.LogDir();

  Shown proxied;
  {
    ClientConnection client;
    const scada::Status connected = client.Connect(proxy.OpcUaUrl(), u"root");
    ASSERT_TRUE(connected) << ::ToString(connected);
    // The downstream link comes up after the proxy listens; the binding
    // resolving is the condition, which OpenDisplay waits for.
    proxied = OpenDisplay(client, vds, binding);
  }
  if (HasFailure() || !proxied.reading)
    proxy.PreserveWorkspace();
  EXPECT_THAT(proxied.data_sources, testing::Contains(binding));
  EXPECT_TRUE(proxied.shape_found);
  ASSERT_TRUE(proxied.reading && proxied.reading->value)
      << "through the proxy: " << Describe(proxied.reading) << "; see "
      << proxy.LogDir();

  // The same live value both times.
  RecordProperty("binding", binding);
  RecordProperty("direct", Describe(direct.reading));
  RecordProperty("proxied", Describe(proxied.reading));
  EXPECT_DOUBLE_EQ(*proxied.reading->value, *direct.reading->value);
  EXPECT_EQ(proxied.reading->text, direct.reading->text);
  proxy.Terminate();
}

// Decision B, through the client itself. The Designer-bound display is
// copied into the client's public folder before sign-in; client_qt signs in
// to open62541 through its own login, lists the file in the main window's
// Display menu, and opening that row shows the live value on the schematic,
// quality Good.
TEST_F(ForeignDisplayE2eTest,
       TheDisplayMenuOpensADesignerBoundDisplayInTheClient) {
  ForeignServer server{std::filesystem::path{SCADA_E2E_FOREIGN_SERVER_EXE},
                       ports_.Allocate(), workdir_ / "server"};
  ASSERT_TRUE(server.started())
      << "open62541 did not start; see " << workdir_ / "server";

  const std::filesystem::path vds = workdir_ / "foreign.vds";
  std::string binding;
  BindAndMoveTheValue(server, vds, binding);
  ASSERT_FALSE(HasFatalFailure());

  // What the operator does by hand: copy the file into the public folder.
  // With --test-data-dir the public folder is that directory.
  const std::filesystem::path data_dir = workdir_ / "ClientData";
  std::filesystem::create_directories(data_dir);
  std::filesystem::copy_file(
      vds, data_dir / (std::string{kPublicFileStem} + ".vds"));

  GuiClient client{
      workdir_ / "client", data_dir,
      server.endpoint_url().substr(std::string_view{"opc.tcp://"}.size())};
  const std::string report = client.WaitForReport(Timeout(120s));
  RecordProperty("report", report);
  // The scratch directory goes with the test, so a failure carries the
  // client's own account of the run.
  const auto context = [&] {
    return "client status '" + client.status() + "'; report:\n" + report +
           "\nclient log tail:\n" + client.LogTail(4000);
  };

  ASSERT_FALSE(report.empty()) << context();
  EXPECT_THAT(ReportValues(report, "row"),
              testing::Contains(std::string{kPublicFileStem}))
      << context();
  EXPECT_THAT(ReportValues(report, "opened-view"),
              testing::ElementsAre("VidiconDisplay"))
      << context();
  EXPECT_THAT(ReportValues(report, "data-source"), testing::Contains(binding))
      << context();

  const std::vector<std::string> readings = ReportValues(report, "reading");
  ASSERT_EQ(readings.size(), 1u) << context();
  const std::optional<ReportedReading> reading =
      ParseReportedReading(readings.front());
  ASSERT_TRUE(reading) << readings.front();
  EXPECT_EQ(reading->data_source, binding);
  EXPECT_EQ(reading->quality, 0) << context();
  ASSERT_TRUE(reading->value) << context();
  EXPECT_DOUBLE_EQ(*reading->value, kLiveValue) << context();
  EXPECT_NE(report.find("display-menu: ok"), std::string::npos) << context();
}

}  // namespace
}  // namespace client::test
