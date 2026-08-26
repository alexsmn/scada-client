#include "base/time/time_wire_codec.h"
#include "test/e2e/client_server_e2e_test_support.h"

#include "base/awaitable.h"
#include "base/time/time.h"
#include "opcua/client/client_session.h"
#include "opcua_bridge/client_adapters.h"
#include "scada/attribute_service.h"
#include "scada/date_time.h"
#include "scada/event.h"
#include "scada/event_filter.h"
#include "scada/history_service.h"
#include "scada/node_management_service.h"
#include "scada/read_value_id.h"
#include "scada/session_service.h"
#include "scada/view_service.h"
#include "transport/transport_factory_impl.h"

#include <boost/asio/io_context.hpp>
#include <boost/json.hpp>

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <thread>
#include <type_traits>
#include <vector>

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

// True if the report lists at least one device and NO device is left in
// state=Unknown. A device whose runtime status never routes through the proxy
// reads Unknown (the device-online regression); a device that is genuinely not
// connected reports Offline — a real value that DID route. So "no Unknown"
// catches the routing gap for EVERY device, a stricter guard than "one active
// device per protocol", without asserting connectivity the loopback fixture
// does not give every device (e.g. the IEC60870 server-side device settles
// Offline).
bool NoHardwareTreeDeviceUnknown(std::string_view report) {
  static constexpr std::string_view kStateMarker = ".state=";
  std::istringstream stream{std::string{report}};
  std::string line;
  int device_count = 0;
  while (std::getline(stream, line)) {
    const auto pos = line.find(kStateMarker);
    if (!line.starts_with("device[") || pos == std::string::npos)
      continue;
    ++device_count;
    if (line.substr(pos + kStateMarker.size()) == "Unknown")
      return false;
  }
  return device_count > 0;
}

// How many tree levels the report settled, counting `label[0]`, `label[1]`, ...
// from the top and stopping at the first index that is absent or empty.
//
// This is the structural half of the object-tree assertion. The named-label
// check below is a *fixture* assertion: it pins the labels the hermetic
// fixture's own dataset produces, which is exactly what makes it the
// regression guard there. Against an already-running deployment those names
// are someone else's data -- the GCP demo settles four levels ending in
// `Дорасчеты / БСК-10 Сумма фаз` -- so the property worth asserting is that
// the tree expanded to depth at all, which is what this counts. Each level is
// a round trip whose result the next level needs, so depth is still the
// throughput signal the case was written for.
int CountSettledObjectTreeLabels(std::string_view report) {
  std::istringstream stream{std::string{report}};
  std::vector<std::string> lines;
  std::string line;
  while (std::getline(stream, line))
    lines.push_back(std::move(line));

  int depth = 0;
  for (;; ++depth) {
    const std::string prefix = "label[" + std::to_string(depth) + "]=";
    auto it = std::find_if(lines.begin(), lines.end(),
                           [&prefix](const std::string& candidate) {
                             return candidate.starts_with(prefix);
                           });
    // Absent, or present but empty: the tree stopped settling here.
    if (it == lines.end() || it->size() == prefix.size())
      return depth;
  }
}

// The depth the object tree is expected to reach. Four levels is what both the
// hermetic fixture and the GCP demo settle, and it is deep enough that the
// round-trip throughput the case guards is actually exercised.
constexpr int kExpectedObjectTreeDepth = 4;

// These need no server, so they run in a build configured without tier
// binaries -- which is the point of that mode existing.
TEST(ObjectTreeLabelsReportTest, CountsConsecutiveSettledLevels) {
  EXPECT_EQ(CountSettledObjectTreeLabels(
                "object-tree-labels: ok\n"
                "detail\n"
                "label[0]=All objects\n"
                "label[1]=Substation\n"
                "label[2]=Group\n"
                "label[3]=Breaker\n"),
            4);
}

// The regression this case exists for: a deployment's own dataset settles the
// same four levels under entirely different names. Asserting the fixture's
// names failed here on data, not on behaviour.
TEST(ObjectTreeLabelsReportTest, CountsDepthRegardlessOfLabelText) {
  EXPECT_EQ(CountSettledObjectTreeLabels("label[0]=Root\n"
                                         "label[1]=Feeder 110\n"
                                         "label[2]=Derived\n"
                                         "label[3]=BSK-10 phase sum\n"),
            4);
}

// "A report with label[0] and nothing below it means the tree never expanded
// at all" -- the failure mode the case was written to catch must still fail.
TEST(ObjectTreeLabelsReportTest, StopsAtTheFirstUnsettledLevel) {
  EXPECT_EQ(CountSettledObjectTreeLabels("label[0]=All objects\n"), 1);
  EXPECT_EQ(CountSettledObjectTreeLabels("label[0]=All objects\n"
                                         "label[1]=\n"
                                         "label[2]=Group\n"),
            1);
  // A gap is not settled either, however much follows it.
  EXPECT_EQ(CountSettledObjectTreeLabels("label[0]=All objects\n"
                                         "label[2]=Group\n"
                                         "label[3]=Breaker\n"),
            1);
}

TEST(ObjectTreeLabelsReportTest, CountsNothingInAReportWithNoLabels) {
  EXPECT_EQ(CountSettledObjectTreeLabels(""), 0);
  EXPECT_EQ(CountSettledObjectTreeLabels("object-tree-labels: failure\n"
                                         "no tree\n"),
            0);
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

// The FileSystem subtree served by the dedicated filesystem tier, addressed
// through the proxy. The root object and FileType live in the shared SCADA
// model namespace; file/directory instances carry string ids in the
// file-instance namespace the proxy claims to the filesystem tier.
const scada::NodeId kFileSystemRoot{304, 7};
const scada::NodeId kFileTypeId{306, 7};
constexpr scada::NamespaceIndex kFileInstanceNs = 24;

// A direct OPC UA session to the cluster proxy. The FileSystem tests assert
// the aggregation seam itself — fan-out Browse merging the filesystem tier's
// children, Read of file contents, and AddNodes/DeleteNodes routed by the
// proxy's exclusive claim — without driving the Qt client UI.
class ProxyOpcUaSession {
 private:
  // Defined before its callers: a deduced-return-type member cannot be used
  // above its definition. Non-void results are captured through an optional —
  // co_spawn's future path requires a default-constructible result, which
  // Status/StatusOr are not.
  template <class Fn>
  auto Run(Fn&& fn) {
    using Result = typename std::invoke_result_t<Fn>::value_type;
    if constexpr (std::is_void_v<Result>) {
      RunAwaitable(io_, std::forward<Fn>(fn));
    } else {
      std::optional<Result> result;
      RunAwaitable(io_,
                   [&]() -> Awaitable<void> { result.emplace(co_await fn()); });
      return std::move(*result);
    }
  }

 public:
  ProxyOpcUaSession()
      : transport_factory_{transport::CreateTransportFactory()},
        session_{std::make_shared<opcua::ClientSession>(io_.get_executor(),
                                                        *transport_factory_)},
        services_{scada::opcua_bridge::CreateClientDataServices(session_)} {}

  ~ProxyOpcUaSession() {
    if (connected_) {
      Run([this] { return services_.session_service_->Disconnect(); });
    }
  }

  scada::Status Connect(int opcua_port) {
    auto status = Run([this, opcua_port] {
      return services_.session_service_->ConnectStatus(
          scada::SessionConnectParams{
              .connection_string =
                  "opc.tcp://127.0.0.1:" + std::to_string(opcua_port),
              .user_name = u"root",
              .password = {}});
    });
    connected_ = static_cast<bool>(status);
    return status;
  }

  scada::StatusOr<std::vector<scada::BrowseResult>> BrowseChildren(
      const scada::NodeId& node_id) {
    return Run([this, &node_id] {
      // A concrete reference type is required — the node managers match
      // requested-vs-actual via IsSubtypeOf, and a null request type matches
      // nothing (real clients browse with Organizes/HierarchicalReferences).
      return services_.view_service_->Browse(
          scada::ServiceContext{},
          {scada::BrowseDescription{
              .node_id = node_id,
              .direction = scada::BrowseDirection::Forward,
              .reference_type_id =
                  scada::NodeId{scada::id::HierarchicalReferences}}});
    });
  }

  scada::DataValue ReadValue(const scada::NodeId& node_id) {
    return Run([this, &node_id] {
      return scada::Read(*services_.attribute_service_, scada::ServiceContext{},
                         scada::ReadValueId{node_id});
    });
  }

  // Mirrors the Qt client's event journal read (HistoricalEventModel):
  // HistoryReadEvents rooted at the Server object. An unset filter (types=0)
  // matches every stored event regardless of type or ack state.
  scada::StatusOr<scada::HistoryReadEventsResult> ReadEventHistory(
      scada::Time from,
      scada::Time to) {
    return Run([this, from, to] {
      return services_.history_service_->HistoryReadEvents(
          scada::NodeId{scada::id::Server}, from, to, scada::EventFilter{});
    });
  }

  scada::StatusOr<std::vector<scada::AddNodesResult>> AddNode(
      scada::AddNodesItem item) {
    return Run([this, &item] {
      std::vector<scada::AddNodesItem> items;
      items.push_back(std::move(item));
      return services_.node_management_service_->AddNodes(
          scada::ServiceContext{}, std::move(items));
    });
  }

  scada::StatusOr<std::vector<scada::StatusCode>> DeleteNode(
      const scada::NodeId& node_id) {
    return Run([this, &node_id] {
      return services_.node_management_service_->DeleteNodes(
          scada::ServiceContext{},
          {scada::DeleteNodesItem{.node_id = node_id}});
    });
  }

 private:
  boost::asio::io_context io_;
  std::shared_ptr<transport::TransportFactory> transport_factory_;
  std::shared_ptr<opcua::ClientSession> session_;
  ::DataServices services_;
  bool connected_ = false;
};

// True if a Browse of the FileSystem root lists a file whose string node id in
// the file-instance namespace equals `name`; fills `node_id` with the match.
// `last_browse` (when set) captures a description of the final Browse outcome
// so a timeout failure shows what the proxy actually returned.
bool FindFileChild(ProxyOpcUaSession& session,
                   std::string_view name,
                   scada::NodeId* node_id,
                   std::string* last_browse = nullptr) {
  auto result = session.BrowseChildren(kFileSystemRoot);
  if (!result.ok()) {
    if (last_browse)
      *last_browse = "Browse failed: " + ::ToString(result.status());
    return false;
  }
  if (result->empty()) {
    if (last_browse)
      *last_browse = "Browse returned no results";
    return false;
  }
  if (last_browse) {
    *last_browse =
        "Browse status " +
        std::to_string(static_cast<int>(result->front().status_code)) +
        ", references:";
    for (const auto& reference : result->front().references) {
      *last_browse += " " + std::string{reference.node_id.ToString()};
    }
  }
  for (const auto& reference : result->front().references) {
    const auto& id = reference.node_id;
    if (id.namespace_index() == kFileInstanceNs &&
        id.type() == scada::NodeIdType::String && id.string_id() == name) {
      *node_id = id;
      return true;
    }
  }
  return false;
}

TEST_P(ClientServerE2eTest, FileSystem_BrowseAndReadThroughProxy) {
  // The seam under test is proxy aggregation of the filesystem tier, which only
  // exists in the Cluster topology; the direct session always speaks OPC UA, so
  // running it once (under the OpcUa parameter) covers it.
  if (Topology() != ServerTopology::Cluster || Protocol() != E2eProtocol::OpcUa)
    GTEST_SKIP() << "FileSystem aggregation exists in the Cluster topology; "
                    "the direct-session check runs once, under OpcUa";
  if (UsesExternalServer())
    GTEST_SKIP() << "asserts against tier workspaces/state the suite owns only "
                    "when it launches the cluster itself";

  StartServer();

  // Seed a file in the filesystem tier's store; its FileWatcher (2s poll) picks
  // it up, so the Browse below retries until the node appears.
  constexpr std::string_view kContents = "hello from the filesystem tier";
  const auto file_dir = filesystem_tier_->WorkspaceDir() / "FileSystem";
  std::filesystem::create_directories(file_dir);
  WriteTextFile(file_dir / "hello.txt", kContents);

  ProxyOpcUaSession session;
  ASSERT_TRUE(session.Connect(opcua_port_));

  scada::NodeId file_id;
  std::string last_browse;
  const auto deadline =
      std::chrono::steady_clock::now() + std::chrono::seconds{20};
  while (!FindFileChild(session, "hello.txt", &file_id, &last_browse) &&
         std::chrono::steady_clock::now() < deadline) {
    std::this_thread::sleep_for(std::chrono::milliseconds{250});
  }
  ASSERT_FALSE(file_id.is_null())
      << "Browse of the FileSystem root via the proxy never listed hello.txt; "
      << last_browse;

  const auto value = session.ReadValue(file_id);
  ASSERT_TRUE(scada::IsGood(value.status_code))
      << "Read of the file contents via the proxy failed: "
      << static_cast<int>(value.status_code);
  const auto* bytes = value.value.get_if<scada::ByteString>();
  ASSERT_TRUE(bytes) << "File Value attribute is not a ByteString";
  EXPECT_EQ(std::string(bytes->begin(), bytes->end()), kContents);
}

TEST_P(ClientServerE2eTest, FileSystem_CreateAndDeleteThroughProxy) {
  if (Topology() != ServerTopology::Cluster || Protocol() != E2eProtocol::OpcUa)
    GTEST_SKIP() << "FileSystem aggregation exists in the Cluster topology; "
                    "the direct-session check runs once, under OpcUa";
  if (UsesExternalServer())
    GTEST_SKIP() << "asserts against tier workspaces/state the suite owns only "
                    "when it launches the cluster itself";

  StartServer();

  ProxyOpcUaSession session;
  ASSERT_TRUE(session.Connect(opcua_port_));

  // Wait for the proxy's downstream link to the filesystem tier (its svc
  // login + namespace read run asynchronously after startup — AddNodes before
  // that reports Bad_Disconnected): the tier's fixture files merge into the
  // root Browse once the link is up.
  scada::NodeId fixture_file;
  const auto link_deadline =
      std::chrono::steady_clock::now() + std::chrono::seconds{20};
  while (!FindFileChild(session, "main.sde", &fixture_file) &&
         std::chrono::steady_clock::now() < link_deadline) {
    std::this_thread::sleep_for(std::chrono::milliseconds{250});
  }
  ASSERT_FALSE(fixture_file.is_null())
      << "the proxy's filesystem downstream link never came up";

  // Mirrors the Qt client's Add-file command: AddNodes of a FileType variable
  // under the FileSystem root, contents in the Value attribute. The proxy must
  // route it to the filesystem tier via the claimed root node.
  constexpr std::string_view kContents = "uploaded through the proxy";
  auto added = session.AddNode(scada::AddNodesItem{
      .parent_id = kFileSystemRoot,
      .node_class = scada::NodeClass::Variable,
      .type_definition_id = kFileTypeId,
      .attributes = scada::NodeAttributes{
          .display_name = scada::LocalizedText{u"upload.txt"},
          .value = scada::ByteString{kContents.begin(), kContents.end()}}});
  ASSERT_TRUE(added.ok()) << "AddNodes via the proxy failed: "
                          << ::ToString(added.status());
  ASSERT_EQ(added->size(), 1u);
  ASSERT_TRUE(scada::IsGood(added->front().status_code))
      << "AddNodes item status: "
      << static_cast<int>(added->front().status_code);
  const scada::NodeId file_id = added->front().added_node_id;
  EXPECT_EQ(file_id.namespace_index(), kFileInstanceNs);

  // The write must land on the filesystem tier's disk — that is the whole
  // point of the extraction.
  const auto file_path =
      filesystem_tier_->WorkspaceDir() / "FileSystem" / "upload.txt";
  const auto write_deadline =
      std::chrono::steady_clock::now() + std::chrono::seconds{10};
  while (!std::filesystem::exists(file_path) &&
         std::chrono::steady_clock::now() < write_deadline) {
    std::this_thread::sleep_for(std::chrono::milliseconds{100});
  }
  ASSERT_TRUE(std::filesystem::exists(file_path))
      << "AddNodes via the proxy did not materialize the file on the "
         "filesystem tier's disk";
  EXPECT_EQ(ReadFileOrEmpty(file_path), kContents);

  // The tier registers the new node on its next watcher pass (~2s); deleting
  // before then reports Bad_WrongNodeId even on a single server. Wait until
  // the file is browsable — the point where a real client would delete it.
  scada::NodeId browsed_id;
  const auto browse_deadline =
      std::chrono::steady_clock::now() + std::chrono::seconds{20};
  while (!FindFileChild(session, "upload.txt", &browsed_id) &&
         std::chrono::steady_clock::now() < browse_deadline) {
    std::this_thread::sleep_for(std::chrono::milliseconds{250});
  }
  ASSERT_FALSE(browsed_id.is_null())
      << "the created file never became browsable via the proxy";

  auto deleted = session.DeleteNode(file_id);
  ASSERT_TRUE(deleted.ok())
      << "DeleteNodes via the proxy failed: " << ::ToString(deleted.status());
  ASSERT_EQ(deleted->size(), 1u);
  EXPECT_TRUE(scada::IsGood(deleted->front()))
      << "DeleteNodes item status: " << static_cast<int>(deleted->front());
  const auto delete_deadline =
      std::chrono::steady_clock::now() + std::chrono::seconds{10};
  while (std::filesystem::exists(file_path) &&
         std::chrono::steady_clock::now() < delete_deadline) {
    std::this_thread::sleep_for(std::chrono::milliseconds{100});
  }
  EXPECT_FALSE(std::filesystem::exists(file_path))
      << "DeleteNodes via the proxy did not remove the file from the "
         "filesystem tier's disk";
}

TEST_P(ClientServerE2eTest, Events_HistoryReadThroughProxy) {
  // Event history is the historian tier's (ADR 0002/0004: edges run no history
  // module; the events module stays framework-embedded and the historian's
  // DataCollector subscribes to system events and files them in its event
  // database). The client-visible path under test: HistoryReadEvents rooted at
  // the Server object → proxy → history-link (the historian registered with
  // the "HD" capability) → historian event DB. Only the Cluster topology has
  // that seam, and the direct session always speaks OPC UA, so it runs once.
  if (Topology() != ServerTopology::Cluster || Protocol() != E2eProtocol::OpcUa)
    GTEST_SKIP() << "event history needs the historian tier behind the proxy; "
                    "the direct-session check runs once, under OpcUa";
  // ProxyOpcUaSession dials 127.0.0.1 — the direct-session checks only address
  // a cluster this process launched.
  if (UsesExternalServer())
    GTEST_SKIP() << "direct proxy session addresses the locally launched "
                    "cluster only";

  // The window must cover the tiers' startup burst of system events (module
  // and device state events raised while the cluster comes up), which is what
  // the historian collects — freeze the start before the tiers exist.
  const scada::Time window_start = scada::Now() - std::chrono::minutes(1);

  StartServer();

  ProxyOpcUaSession session;
  ASSERT_TRUE(session.Connect(opcua_port_));

  // The historian's event subscription and its write batching are
  // asynchronous; poll until stored events come back through the proxy.
  scada::StatusOr<scada::HistoryReadEventsResult> result{
      scada::StatusCode::Bad};
  const auto deadline =
      std::chrono::steady_clock::now() + std::chrono::seconds{30};
  while (std::chrono::steady_clock::now() < deadline) {
    result = session.ReadEventHistory(window_start,
                                      scada::Now() + std::chrono::minutes(1));
    if (result.ok() && !result->events.empty())
      break;
    std::this_thread::sleep_for(std::chrono::milliseconds{500});
  }
  ASSERT_TRUE(result.ok()) << "HistoryReadEvents via the proxy failed: "
                           << ::ToString(result.status());
  ASSERT_FALSE(result->events.empty())
      << "HistoryReadEvents via the proxy returned no events: the historian "
         "collected no system events from the cluster startup, or the "
         "proxy's history link did not route the read to it";

  // Every returned event must be a well-formed, fully-populated record — this
  // locks the events OPC UA alignment invariants across the wire and the
  // historian's store: a non-empty event id, a resolvable type, a source node,
  // timestamps inside the requested window, and Severity in the OPC UA 1..1000
  // range (Part 5 §6.4.2).
  for (const scada::Event& event : result->events) {
    EXPECT_TRUE(event.is_valid())
        << "stored event is not valid; type " << event.event_type_id.ToString();
    EXPECT_FALSE(event.event_type_id.is_null());
    EXPECT_FALSE(event.source_node_id.is_null())
        << "event has no source node; type " << event.event_type_id.ToString();
    EXPECT_GE(event.severity, 1u);
    EXPECT_LE(event.severity, 1000u);
    EXPECT_GE(event.time, window_start)
        << "event time precedes the requested window";
  }
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
  // Runs under every topology. Cluster used to be skipped for "nested
  // station/group DisplayNames don't come through aggregation"; that premise no
  // longer holds — retested 2026-08-02 with the skip removed, both Cluster
  // parameterizations passed 4/4 runs each at a steady ~30 s.
  //
  // This case is the suite's only assertion on sustained round-trip throughput:
  // each tree level is a round trip whose result the next level needs, so it
  // fails outright — not slowly — when the client cannot keep up with the
  // server's notification stream. That is what it caught on 2026-08-02 (both
  // remote read loops resized a 16 MiB buffer per message; see
  // docs/client/message-loop.md §9). A failure reporting label[0] and nothing
  // below it means the tree never expanded at all, not that it expanded slowly.
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
  if (UsesExternalServer()) {
    // A deployment carries its own dataset, so the fixture's names are not a
    // property of a working client. Assert the structure instead: four levels
    // deep and every one of them settled. See CountSettledObjectTreeLabels.
    EXPECT_GE(CountSettledObjectTreeLabels(report), kExpectedObjectTreeDepth)
        << "Object tree did not expand " << kExpectedObjectTreeDepth
        << " settled levels against the external deployment; a report with "
           "label[0] and nothing below it means it never expanded at all:\n"
        << report;
  } else {
    for (std::string_view expected_label :
         {"label[0]=Все объекты", "label[1]=Отрадная 110 КВ", "label[2]=ТС",
          "label[3]=МВ-35 У"}) {
      EXPECT_NE(report.find(expected_label), std::string::npos)
          << "Missing expected object tree label " << expected_label
          << " in report:\n"
          << report;
    }
  }

  ExpectServerAuthLog();
  ExpectProcessesRemainRunningFor(
      std::chrono::duration_cast<std::chrono::milliseconds>(
          kPostConnectStabilityTimeout),
      "waiting for expanded object tree labels to remain available");
}

TEST_P(ClientServerE2eTest, Connect_Success_ExpandsHardwareTreeDevices) {
  // The hardware tree asserts a *live* device per protocol (MODBUS + IEC60870 +
  // IEC61850). Multi-protocol coverage needs all three device edges, and only
  // the Cluster topology stands them up (the SingleTier fixture configures
  // IEC60870 devices alone), so this test is Cluster-only.
  //
  // Device-status routing through the aggregating proxy is fixed end to end:
  // every device (both instances of MODBUS, IEC60870 and IEC61850) reaches
  // state=Online through the proxy. The path had two masks: the client reading
  // status by its nested node id (`MakeNestedNodeId(device, "Online")`) rather
  // than a synchronous aggregate lookup that returned null against a remote
  // node service, and — the last server-side piece — per-tier served
  // NamespaceArrays (ADR 0003), which stopped an edge from forwarding a
  // device's runtime-status monitored item to the config tier (which has no
  // such node → Bad_WrongNodeId → Unknown). The client-side capture asserts
  // EVERY device resolved its status (never Unknown), not one per protocol, so
  // a single device left Unknown fails the test. Offline is allowed — it is a
  // value that routed (the IEC60870 server-side device has no peer and settles
  // Offline).
  //
  // Run the cluster suite with SCADA_SERVER_LICENSE_REQUIRE_GCP_BINDING=false;
  // without it every tier stops during startup and the harness reports it as
  // "did not start listening", which reads like a timeout rather than a licence
  // refusal.
  if (Topology() != ServerTopology::Cluster)
    GTEST_SKIP() << "multi-protocol hardware tree requires the Cluster "
                    "topology (SingleTier configures only IEC60870)";

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
  if (!UsesExternalServer()) {
    // Which protocols are present is the fixture's own topology: it stands up
    // one live device of each. A deployment runs whatever it runs -- the GCP
    // demo's four devices are all IEC60870 -- so requiring a fixed protocol set
    // there fails on dataset rather than on behaviour. The "no Unknown" check
    // below is the part that holds against any deployment, and it is the
    // stronger of the two.
    for (std::string_view protocol : {"MODBUS", "IEC60870", "IEC61850"}) {
      EXPECT_TRUE(HasActiveHardwareTreeDevice(report, protocol))
          << "Hardware tree device for " << protocol
          << " was not active in report:\n"
          << report;
    }
  }
  // Every device — of every protocol — must have a resolved runtime status
  // (never Unknown), not just one active per protocol. This is the actual
  // regression guard: a device whose status fails to route through the proxy
  // reads state=Unknown, and the earlier "one active per protocol" check passed
  // as long as one sibling was up. Offline is allowed — it is a real value that
  // routed (the IEC60870 server-side device has no peer and settles Offline).
  EXPECT_TRUE(NoHardwareTreeDeviceUnknown(report))
      << "A hardware-tree device was left state=Unknown, meaning its runtime "
         "status did not route through the proxy:\n"
      << report;

  ExpectServerAuthLog();
  ExpectProcessesRemainRunningFor(
      std::chrono::duration_cast<std::chrono::milliseconds>(
          kPostConnectStabilityTimeout),
      "waiting for expanded hardware tree devices to remain available");
}

TEST_P(ClientServerE2eTest, Connect_Success_DisplaysHistoricalTimedData) {
  // History is the historian tier's, not a device tier's (ADR 0002: edges own
  // no history), so this runs only under the Cluster topology. End-to-end path:
  // the config tier + historian carry TIT.4's historization; the edges load +
  // activate + simulate it (remote-config enumeration + reference
  // reconstruction); the historian pull-collects it from the edge
  // (historyCollection.sources) and the client reads it back through the
  // proxy's history link to the historian (the historian registers with the
  // "HD" capability; the proxy's history-link module routes ALL history to
  // it). Skipped under SingleTier (no historian).
  if (Topology() != ServerTopology::Cluster)
    GTEST_SKIP() << "history needs the historian tier (a single device tier "
                    "owns no history)";
  // The window this asserts over is created by EnableSimulatedHistory(), which
  // historizes TIT.4 in the config tier's and historian's DBs before launch —
  // fixture control the suite does not have over an external deployment.
  if (UsesExternalServer())
    GTEST_SKIP() << "historized-item fixture cannot be seeded in an external "
                    "deployment's config/historian databases";
  WriteClientSettings(/*password=*/"");
  EnableSimulatedHistory();
  StartServer();
  // Freeze the historical window's end BEFORE the client exists: every live
  // monitored-item update the client will buffer carries a source timestamp
  // after this instant, so only rows served by a HistoryRead against the
  // historian's store (collected while the tiers were starting up) can land
  // inside the window. Without this the view's default Day window let
  // client-side live buffering populate the rows and the test passed even
  // when proxy history routing was broken.
  const int64_t history_window_end =
      scada::base::EncodeWireMicroseconds(scada::Now());
  StartClient({"--test-historical-timed-data-file=" +
                   historical_timed_data_file_.string(),
               "--test-historical-timed-data-end=" +
                   std::to_string(history_window_end)});

  ASSERT_TRUE(WaitForStartupOrStatus())
      << "Timed out waiting for client startup/status signal";
  const auto status = ReadFileOrEmpty(status_file_);
  ASSERT_TRUE(ContainsInDirectory(client_log_dir_, kStartupCompletedLog))
      << "Client did not log startup completion; status: " << status;
  ASSERT_TRUE(status.empty() || status == "success")
      << "Unexpected client status while waiting for startup: " << status;
  ASSERT_TRUE(client_.IsRunning()) << "Client exited unexpectedly after login";

  // The historian collects the simulated TIT.4 into its store; the client
  // opens the real timed-data view over the frozen pre-launch window, reads
  // those samples back through the active backend (via the proxy's history
  // link to the historian), and exports them via the view's real
  // Export-to-CSV writer. The CSV is a header row plus one row per sample, so
  // at least one newline (one data row) proves historian-served rows populated
  // the view — live buffering cannot land inside the window.
  const auto report = WaitForHistoricalTimedDataReport();
  EXPECT_GT(std::count(report.begin(), report.end(), '\n'), 0)
      << "Expected the exported timed-data CSV to contain historical rows:\n"
      << report;

  ExpectServerAuthLog();
  ExpectProcessesRemainRunningFor(
      std::chrono::duration_cast<std::chrono::milliseconds>(
          kPostConnectStabilityTimeout),
      "waiting for the historical timed-data view to remain available");
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
  // Profile writes route client→proxy→edge→config tier; that remote-config
  // write-through isn't wired through the cluster yet (a pending server-tier
  // gap). Runs under SingleTier where the profile is written to the single
  // tier's own config DB.
  if (Topology() == ServerTopology::Cluster)
    GTEST_SKIP() << "profile write-through pending a server-tier gap "
                    "(remote-config write routing)";

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
  // Only meaningful for a locally launched server — an external deployment's
  // logs are not in the workspace, where the check would pass vacuously.
  if (!UsesExternalServer()) {
    EXPECT_FALSE(
        ContainsInDirectory(server_log_dir_, "Authorization succeeded"))
        << "Server should not record successful authorization";
  }
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
  if (!UsesExternalServer()) {
    EXPECT_FALSE(
        ContainsInDirectory(server_log_dir_, "OPC UA session activated"))
        << "Server should not activate a session when security selection fails";
  }
  ExpectServerRemainsRunningFor(
      std::chrono::duration_cast<std::chrono::milliseconds>(
          kPostConnectStabilityTimeout),
      "waiting for the server to remain stable after rejecting an unsupported "
      "security mode");
}

INSTANTIATE_TEST_SUITE_P(
    Protocols,
    ClientServerE2eTest,
    // Full matrix: each backend protocol against both server topologies — one
    // device tier (SingleTier) and the real tier split behind an aggregating
    // proxy (Cluster). The two multi-protocol/history tests self-skip under
    // SingleTier (see their bodies); everything else runs under both, proving
    // the client behaves identically one-process or clustered.
    ::testing::Values(E2eParam{E2eProtocol::Remote, ServerTopology::SingleTier},
                      E2eParam{E2eProtocol::OpcUa, ServerTopology::SingleTier},
                      E2eParam{E2eProtocol::Remote, ServerTopology::Cluster},
                      E2eParam{E2eProtocol::OpcUa, ServerTopology::Cluster}),
    [](const ::testing::TestParamInfo<E2eParam>& info) {
      return E2eParamName(info.param);
    });

}  // namespace
}  // namespace client::test
