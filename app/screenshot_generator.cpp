#include "client_application.h"

#include "aui/test/app_environment.h"
#include "configuration/tree/node_service_tree_mock.h"
#include "controller/window_info.h"
#include "base/client_paths.h"
#include "base/test/scoped_path_override.h"
#include "base/test/test_executor.h"
#include "main_window/main_window.h"
#include "main_window/main_window_manager.h"
#include "main_window/opened_view.h"
#include "node_service/node_model_mock.h"
#include "profile/profile.h"
#include "scada/attribute_service_mock.h"
#include "scada/history_service_mock.h"
#include "scada/services_mock.h"
#include "scada/view_service_mock.h"

#include <QApplication>
#include <QPixmap>
#include <boost/asio/io_context.hpp>
#include <gmock/gmock.h>

#include <filesystem>
#include <random>

using namespace ::testing;

namespace {

// Output directory for screenshots. Override via --screenshot-dir flag.
std::filesystem::path g_output_dir;

// Window types to capture. Matches kKnownWindowTypes in
// client_application_unittest.
struct ScreenshotSpec {
  std::string_view window_type;
  std::string_view filename;
  int width = 800;
  int height = 600;
};

constexpr ScreenshotSpec kScreenshots[] = {
    {"Graph", "graph.png", 900, 500},
    {"Table", "table.png", 800, 400},
    {"Summ", "summary.png", 800, 500},
    {"Event", "events.png", 800, 300},
    {"EventJournal", "events-log.png", 800, 400},
    {"Log", "device-watch.png", 800, 400},
    {"Nodes", "object-tree.png", 300, 500},
    {"Struct", "devices.png", 300, 500},
    {"Users", "users.png", 600, 300},
    {"Params", "parameters.png", 500, 400},
    {"CusTable", "sheet.png", 800, 400},
    {"Favorites", "favorites.png", 300, 400},
    {"FileSystemView", "files.png", 300, 400},
    {"TimeVal", "data.png", 800, 400},
    {"Transmission", "retransmission.png", 800, 400},
};

Page MakeScreenshotPage() {
  Page page;
  for (const auto& spec : kScreenshots) {
    page.AddWindow(WindowDefinition{spec.window_type});
  }
  return page;
}

std::filesystem::path GetOutputDir() {
  if (!g_output_dir.empty())
    return g_output_dir;
  // Default: screenshots/ in the current working directory.
  return std::filesystem::current_path() / "screenshots";
}

void SaveScreenshot(QWidget* widget, const ScreenshotSpec& spec) {
  if (!widget)
    return;

  widget->resize(spec.width, spec.height);
  // Process pending layout events so the widget renders at the new size.
  QApplication::processEvents();

  QPixmap pixmap = widget->grab();
  auto path = GetOutputDir() / spec.filename;
  pixmap.save(QString::fromStdString(path.string()));
}

// --- Mock node tree helpers ---

NodeRef MakeTestNode(const scada::NodeId& node_id,
                     std::u16string display_name,
                     scada::NodeClass node_class = scada::NodeClass::Object) {
  auto model = std::make_shared<NiceMock<MockNodeModel>>();
  ON_CALL(*model, GetAttribute(scada::AttributeId::NodeId))
      .WillByDefault(Return(scada::Variant{node_id}));
  ON_CALL(*model, GetAttribute(scada::AttributeId::DisplayName))
      .WillByDefault(
          Return(scada::Variant{scada::LocalizedText{display_name}}));
  ON_CALL(*model, GetAttribute(scada::AttributeId::NodeClass))
      .WillByDefault(
          Return(scada::Variant{static_cast<scada::Int32>(node_class)}));
  ON_CALL(*model, GetStatus())
      .WillByDefault(Return(scada::StatusCode::Good));
  ON_CALL(*model, GetFetchStatus())
      .WillByDefault(Return(NodeFetchStatus::NodeAndChildren()));
  return model;
}

std::unique_ptr<NodeServiceTree> MakeMockNodeServiceTree(
    NodeServiceTreeImplContext&&) {
  auto tree = std::make_unique<NiceMock<MockNodeServiceTree>>();

  // Build a hierarchy: Root → 3 substations → 2 devices each.
  static auto root = MakeTestNode({84, 0}, u"Objects");
  static auto sub1 = MakeTestNode({100, 1}, u"Substation Alpha");
  static auto sub2 = MakeTestNode({101, 1}, u"Substation Beta");
  static auto sub3 = MakeTestNode({102, 1}, u"Substation Gamma");
  static auto dev1 = MakeTestNode({103, 1}, u"RTU-01 IEC104");
  static auto dev2 = MakeTestNode({104, 1}, u"RTU-02 Modbus");
  static auto dev3 = MakeTestNode({105, 1}, u"RTU-03 IEC61850");
  static auto dev4 = MakeTestNode({106, 1}, u"Power Meter PM-01");
  static auto dev5 = MakeTestNode({107, 1}, u"Power Meter PM-02");
  static auto dev6 = MakeTestNode({108, 1}, u"Transformer T1");
  static auto var1 =
      MakeTestNode({200, 1}, u"Active Power", scada::NodeClass::Variable);
  static auto var2 =
      MakeTestNode({201, 1}, u"Reactive Power", scada::NodeClass::Variable);
  static auto var3 =
      MakeTestNode({202, 1}, u"Voltage L1-L2", scada::NodeClass::Variable);
  static auto var4 =
      MakeTestNode({203, 1}, u"Current L1", scada::NodeClass::Variable);

  ON_CALL(*tree, GetRoot()).WillByDefault(Return(root));

  ON_CALL(*tree, HasChildren(_)).WillByDefault(Return(false));
  ON_CALL(*tree, HasChildren(root)).WillByDefault(Return(true));
  ON_CALL(*tree, HasChildren(sub1)).WillByDefault(Return(true));
  ON_CALL(*tree, HasChildren(sub2)).WillByDefault(Return(true));
  ON_CALL(*tree, HasChildren(sub3)).WillByDefault(Return(true));
  ON_CALL(*tree, HasChildren(dev1)).WillByDefault(Return(true));

  using ChildRef = NodeServiceTree::ChildRef;
  auto org = scada::id::Organizes;

  ON_CALL(*tree, GetChildren(_))
      .WillByDefault(Return(std::vector<ChildRef>{}));
  ON_CALL(*tree, GetChildren(root))
      .WillByDefault(Return(std::vector<ChildRef>{
          {org, true, sub1}, {org, true, sub2}, {org, true, sub3}}));
  ON_CALL(*tree, GetChildren(sub1))
      .WillByDefault(Return(
          std::vector<ChildRef>{{org, true, dev1}, {org, true, dev2}}));
  ON_CALL(*tree, GetChildren(sub2))
      .WillByDefault(Return(
          std::vector<ChildRef>{{org, true, dev3}, {org, true, dev4}}));
  ON_CALL(*tree, GetChildren(sub3))
      .WillByDefault(Return(
          std::vector<ChildRef>{{org, true, dev5}, {org, true, dev6}}));
  ON_CALL(*tree, GetChildren(dev1))
      .WillByDefault(Return(std::vector<ChildRef>{
          {org, true, var1},
          {org, true, var2},
          {org, true, var3},
          {org, true, var4}}));

  return tree;
}

// --- Mock data helpers ---

scada::DataValue MakeValue(scada::Variant value) {
  const auto now = base::Time::Now();
  return scada::DataValue{std::move(value), {}, now, now};
}

scada::DataValue MakeValueAt(scada::Variant value, base::Time time) {
  return scada::DataValue{std::move(value), {}, time, time};
}

scada::Event MakeEvent(scada::EventId id,
                       base::Time time,
                       scada::UInt32 severity,
                       std::u16string message,
                       scada::NodeId node_id = {},
                       scada::UInt32 change_mask = 0) {
  scada::Event e;
  e.event_id = id;
  e.time = time;
  e.receive_time = time;
  e.severity = severity;
  e.message = std::move(message);
  e.node_id = std::move(node_id);
  e.change_mask = change_mask;
  // Historical event query requests acked events only.
  e.acked = true;
  e.acknowledged_time = time;
  return e;
}

// Generate a simple node hierarchy for browse results.
scada::BrowseResult MakeBrowseChildren(
    std::initializer_list<scada::NodeId> children) {
  scada::BrowseResult result;
  result.status_code = scada::StatusCode::Good;
  for (const auto& child : children) {
    result.references.push_back(
        {.reference_type_id = scada::NodeId{35, 0},
         .forward = true,
         .node_id = child});
  }
  return result;
}

}  // namespace

class ScreenshotGenerator : public Test {
 public:
  ScreenshotGenerator();
  ~ScreenshotGenerator();

 protected:
  void SetupMockData();

  boost::asio::io_context io_context_;
  std::shared_ptr<Executor> executor_ = std::make_shared<TestExecutor>();
  AppEnvironment app_env_;

  NiceMock<scada::MockAttributeService> attribute_service_;
  NiceMock<scada::MockViewService> view_service_;
  NiceMock<scada::MockHistoryService> history_service_;
  NiceMock<scada::MockMonitoredItemService> monitored_item_service_;
  NiceMock<scada::MockSessionService> session_service_;

  scada::services services_{
      .attribute_service = &attribute_service_,
      .monitored_item_service = &monitored_item_service_,
      .history_service = &history_service_,
      .view_service = &view_service_,
      .session_service = &session_service_,
  };

  base::ScopedPathOverride private_dir_override_{client::DIR_PRIVATE};

  NiceMock<MockFunction<promise<std::optional<DataServices>>(
      DataServicesContext&& services_context)>>
      login_handler_;

  ClientApplication app_{ClientApplicationContext{
      .io_context_ = io_context_,
      .executor_ = executor_,
      .login_handler_ = login_handler_.AsStdFunction(),
      .node_service_tree_factory_ = MakeMockNodeServiceTree}};

};

ScreenshotGenerator::ScreenshotGenerator() {
  // Match the default client style.
  QApplication::setStyle("Fusion");

  // Don't actually show windows on screen -- render offscreen only.
  MainWindow::SetHideForTesting();

  ON_CALL(session_service_, HasPrivilege(_)).WillByDefault(Return(true));
  ON_CALL(session_service_, Disconnect())
      .WillByDefault(Return(make_resolved_promise()));

  ON_CALL(monitored_item_service_, CreateMonitoredItem(_, _))
      .WillByDefault(
          [](const scada::ReadValueId&, const scada::MonitoringParameters&) {
            auto item =
                std::make_shared<NiceMock<scada::MockMonitoredItem>>();
            ON_CALL(*item, Subscribe(_))
                .WillByDefault(
                    [](scada::MonitoredItemHandler handler) {
                      // Deliver an initial value so views show data.
                      if (auto* h =
                              std::get_if<scada::DataChangeHandler>(&handler)) {
                        static std::mt19937 rng{99};
                        std::uniform_real_distribution<double> dist(10, 200);
                        (*h)(MakeValue(scada::Variant{dist(rng)}));
                      }
                    });
            return item;
          });

  ON_CALL(login_handler_, Call(/*services_context=*/_))
      .WillByDefault(Return(make_resolved_promise(
          std::optional{DataServices::FromUnownedServices(services_)})));

  SetupMockData();
}

void ScreenshotGenerator::SetupMockData() {
  const auto now = base::Time::Now();

  // --- Attribute reads: return display names and values for nodes ---
  ON_CALL(attribute_service_, Read(_, _, _))
      .WillByDefault(
          [now](const scada::ServiceContext&,
                const std::shared_ptr<const std::vector<scada::ReadValueId>>&
                    inputs,
                const scada::ReadCallback& callback) {
            std::vector<scada::DataValue> results;
            results.reserve(inputs->size());
            for (const auto& input : *inputs) {
              auto id = input.node_id.is_numeric()
                            ? input.node_id.numeric_id()
                            : 0u;
              switch (input.attribute_id) {
                case scada::AttributeId::DisplayName:
                  if (id >= 100 && id < 200) {
                    // Device nodes.
                    static const char* kDeviceNames[] = {
                        "Substation Alpha",  "Substation Beta",
                        "Substation Gamma",  "RTU-01 IEC104",
                        "RTU-02 Modbus",     "RTU-03 IEC61850",
                        "Power Meter PM-01", "Power Meter PM-02",
                        "Transformer T1",    "Transformer T2",
                    };
                    auto idx = id - 100;
                    if (idx < std::size(kDeviceNames))
                      results.push_back(
                          MakeValue(scada::Variant{kDeviceNames[idx]}));
                    else
                      results.push_back(
                          MakeValue(scada::Variant{"Device " +
                                                   std::to_string(id)}));
                  } else if (id >= 200 && id < 300) {
                    // Object/variable nodes.
                    static const char* kVarNames[] = {
                        "Active Power",
                        "Reactive Power",
                        "Voltage L1-L2",
                        "Voltage L2-L3",
                        "Voltage L3-L1",
                        "Current L1",
                        "Current L2",
                        "Current L3",
                        "Frequency",
                        "Power Factor",
                        "Total Energy",
                        "Temperature",
                    };
                    auto idx = id - 200;
                    if (idx < std::size(kVarNames))
                      results.push_back(
                          MakeValue(scada::Variant{kVarNames[idx]}));
                    else
                      results.push_back(MakeValue(
                          scada::Variant{"Var " + std::to_string(id)}));
                  } else {
                    results.push_back(
                        MakeValue(scada::Variant{"Node " +
                                                 std::to_string(id)}));
                  }
                  break;

                case scada::AttributeId::Value: {
                  // Return simulated measurement values.
                  static std::mt19937 rng{42};
                  if (id >= 200 && id < 212) {
                    static const double kBaseValues[] = {
                        125.4, -32.1, 10.52, 10.48, 10.55, 302.5,
                        298.1, 305.7, 50.01, 0.95,  48723, 42.3,
                    };
                    auto idx = id - 200;
                    std::normal_distribution<double> dist(kBaseValues[idx],
                                                         kBaseValues[idx] *
                                                             0.01);
                    results.push_back(MakeValue(scada::Variant{dist(rng)}));
                  } else {
                    results.push_back(MakeValue(scada::Variant{0.0}));
                  }
                  break;
                }

                case scada::AttributeId::NodeClass:
                  if (id >= 200)
                    results.push_back(MakeValue(static_cast<scada::Int32>(2)));
                  else
                    results.push_back(MakeValue(static_cast<scada::Int32>(1)));
                  break;

                default:
                  results.push_back(MakeValue(scada::Variant{}));
                  break;
              }
            }
            callback(scada::StatusCode::Good, std::move(results));
          });

  // --- Browse: return a tree of devices and variables ---
  ON_CALL(view_service_, Browse(_, _, _))
      .WillByDefault(
          [](const scada::ServiceContext&,
             const std::vector<scada::BrowseDescription>& inputs,
             const scada::BrowseCallback& callback) {
            std::vector<scada::BrowseResult> results;
            for (const auto& desc : inputs) {
              auto id = desc.node_id.is_numeric()
                            ? desc.node_id.numeric_id()
                            : 0u;
              auto ns = desc.node_id.namespace_index();
              if (id == 84 && ns == 0) {
                // Objects folder (NS0.84) → substations.
                results.push_back(MakeBrowseChildren({
                    scada::NodeId{100, 1},
                    scada::NodeId{101, 1},
                    scada::NodeId{102, 1},
                }));
              } else if (id == 24 && ns == 1) {
                // Devices root (SCADA.24) → substations.
                results.push_back(MakeBrowseChildren({
                    scada::NodeId{100, 1},
                    scada::NodeId{101, 1},
                    scada::NodeId{102, 1},
                }));
              } else if (id == 304 && ns == 1) {
                // Another root (SCADA.304) → some items.
                results.push_back(MakeBrowseChildren({
                    scada::NodeId{100, 1},
                    scada::NodeId{101, 1},
                }));
              } else if (id >= 100 && id < 103 && ns == 1) {
                // Substation → RTUs/meters.
                results.push_back(MakeBrowseChildren({
                    scada::NodeId{static_cast<uint32_t>(103 + (id - 100) * 2),
                                 1},
                    scada::NodeId{static_cast<uint32_t>(104 + (id - 100) * 2),
                                 1},
                }));
              } else if (id >= 103 && id < 112 && ns == 1) {
                // RTU/meter → variables.
                uint32_t base_var = 200 + (id - 103) * 4;
                results.push_back(MakeBrowseChildren({
                    scada::NodeId{base_var, 1},
                    scada::NodeId{base_var + 1, 1},
                    scada::NodeId{base_var + 2, 1},
                    scada::NodeId{base_var + 3, 1},
                }));
              } else {
                results.push_back(scada::BrowseResult{});
              }
            }
            callback(scada::StatusCode::Good, std::move(results));
          });

  ON_CALL(view_service_, TranslateBrowsePaths(_, _))
      .WillByDefault(InvokeArgument<1>(
          scada::StatusCode::Good,
          std::vector<scada::BrowsePathResult>{}));

  // --- History: return time-series data ---
  ON_CALL(history_service_, HistoryReadRaw(_, _))
      .WillByDefault(
          [now](const scada::HistoryReadRawDetails&,
                const scada::HistoryReadRawCallback& callback) {
            std::vector<scada::DataValue> values;
            std::mt19937 rng{123};
            std::normal_distribution<double> dist(120.0, 15.0);
            for (int i = 0; i < 24; ++i) {
              auto time = now - base::TimeDelta::FromHours(24 - i);
              values.push_back(MakeValueAt(scada::Variant{dist(rng)}, time));
            }
            callback(scada::HistoryReadRawResult{
                .status = scada::StatusCode::Good,
                .values = std::move(values),
            });
          });

  // --- History events ---
  ON_CALL(history_service_, HistoryReadEvents(_, _, _, _, _))
      .WillByDefault(
          [now](const scada::NodeId&, base::Time, base::Time,
                const scada::EventFilter&,
                const scada::HistoryReadEventsCallback& callback) {
            std::vector<scada::Event> events;
            events.push_back(MakeEvent(
                1001, now - base::TimeDelta::FromHours(5), scada::kSeverityNormal,
                u"RTU-01 connected", scada::NodeId{103, 1},
                scada::Event::EVT_SUBS));
            events.push_back(MakeEvent(
                1002, now - base::TimeDelta::FromHours(4), scada::kSeverityWarning,
                u"Voltage L1-L2 high limit exceeded (10.8 kV)",
                scada::NodeId{202, 1}, scada::Event::EVT_LIM));
            events.push_back(MakeEvent(
                1003, now - base::TimeDelta::FromHours(3), scada::kSeverityCritical,
                u"RTU-02 communication failure", scada::NodeId{104, 1},
                scada::Event::EVT_SUBS));
            events.push_back(MakeEvent(
                1004, now - base::TimeDelta::FromHours(2), scada::kSeverityNormal,
                u"RTU-02 communication restored", scada::NodeId{104, 1},
                scada::Event::EVT_SUBS));
            events.push_back(MakeEvent(
                1005, now - base::TimeDelta::FromHours(1), scada::kSeverityNormal,
                u"Manual value write: Active Power = 130.5 MW",
                scada::NodeId{200, 1}, scada::Event::EVT_MAN));
            events.push_back(MakeEvent(
                1006, now - base::TimeDelta::FromMinutes(30), scada::kSeverityWarning,
                u"Power Factor below threshold (0.87)",
                scada::NodeId{209, 1}, scada::Event::EVT_LIM));
            events.push_back(MakeEvent(
                1007, now - base::TimeDelta::FromMinutes(10), scada::kSeverityNormal,
                u"Transformer T1 temperature normal (42.3\u00B0C)",
                scada::NodeId{211, 1}, scada::Event::EVT_VAL));
            callback(scada::HistoryReadEventsResult{
                .status = scada::StatusCode::Good,
                .events = std::move(events),
            });
          });
}

ScreenshotGenerator::~ScreenshotGenerator() {
  app_.Quit().get();
}

#if !defined(UI_WT)

TEST_F(ScreenshotGenerator, CaptureAllWindows) {
  auto output_dir = GetOutputDir();
  std::filesystem::create_directories(output_dir);

  // Set up profile with all window types.
  {
    Profile profile;
    profile.AddPage(MakeScreenshotPage());
    profile.Save();
  }

  app_.Start().get();

  // Let async data loads and model updates complete.
  for (int i = 0; i < 20; ++i)
    QApplication::processEvents();

  const auto& main_windows = app_.main_window_manager().main_windows();
  ASSERT_THAT(main_windows, SizeIs(1));
  const MainWindow& main_window = main_windows.front();

  int captured = 0;
  for (const auto& spec : kScreenshots) {
    OpenedView* view = nullptr;
    for (OpenedView* v : main_window.opened_views()) {
      if (v->window_info().name == spec.window_type) {
        view = v;
        break;
      }
    }

    if (!view) {
      ADD_FAILURE() << "Window type not found: " << spec.window_type;
      continue;
    }

    QWidget* widget = view->view();
    if (!widget) {
      ADD_FAILURE() << "No QWidget for: " << spec.window_type;
      continue;
    }

    SaveScreenshot(widget, spec);
    ++captured;
  }

  std::cout << "Captured " << captured << "/" << std::size(kScreenshots)
            << " screenshots to " << output_dir.string() << std::endl;
}

TEST_F(ScreenshotGenerator, CaptureMainWindow) {
  auto output_dir = GetOutputDir();
  std::filesystem::create_directories(output_dir);

  // Open views that render well with mock data.
  {
    Profile profile;
    Page page;
    page.AddWindow(WindowDefinition{"EventJournal"});
    page.AddWindow(WindowDefinition{"Summ"});
    page.AddWindow(WindowDefinition{"Struct"});
    profile.AddPage(page);
    profile.Save();
  }

  app_.Start().get();

  for (int i = 0; i < 20; ++i)
    QApplication::processEvents();

  const auto& main_windows = app_.main_window_manager().main_windows();
  ASSERT_THAT(main_windows, SizeIs(1));

  // The MainWindow is a QMainWindow -- grab the whole window.
  auto* qmain = dynamic_cast<const QWidget*>(&main_windows.front());
  if (qmain) {
    auto* mutable_widget = const_cast<QWidget*>(qmain);
    mutable_widget->resize(1024, 700);
    QApplication::processEvents();
    QPixmap pixmap = mutable_widget->grab();
    auto path = output_dir / "client-window.png";
    pixmap.save(QString::fromStdString(path.string()));
  }
}

#endif
