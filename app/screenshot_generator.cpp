#include "client_application.h"

#include "app/qt/installed_style.h"
#include "app/qt/installed_translation.h"
#include "aui/test/app_environment.h"
#include "base/boost_json_file.h"
#include "base/utf_convert.h"
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
#include "timed_data/timed_data_service_fake.h"

#include <QPixmap>
#include <boost/asio/io_context.hpp>
#include <boost/json.hpp>
#include <gmock/gmock.h>

#include <filesystem>
#include <random>

using namespace ::testing;

namespace {

// Output directory for screenshots. Override via --screenshot-dir flag.
std::filesystem::path g_output_dir;

// --- JSON data loaded at startup ---

struct ScreenshotData {
  boost::json::value json;

  // Parsed from JSON.
  struct NodeInfo {
    scada::NodeId node_id;
    std::string name;
    scada::NodeClass node_class = scada::NodeClass::Object;
    double base_value = 0.0;
    bool has_base_value = false;
  };

  struct ScreenshotSpec {
    std::string window_type;
    std::string filename;
    int width = 800;
    int height = 600;
  };

  std::vector<NodeInfo> nodes;
  std::map<std::string, std::vector<uint32_t>> tree;
  std::vector<ScreenshotSpec> screenshots;

  void Load(const std::filesystem::path& path) {
    auto opt = ReadBoostJsonFromFile(path);
    ASSERT_TRUE(opt.has_value()) << "Failed to read " << path.string();
    json = std::move(*opt);
    ParseNodes();
    ParseTree();
    ParseScreenshots();
  }

  const NodeInfo* FindNode(uint32_t id) const {
    for (const auto& n : nodes)
      if (n.node_id.numeric_id() == id)
        return &n;
    return nullptr;
  }

 private:
  void ParseNodes() {
    for (const auto& jn : json.at("nodes").as_array()) {
      NodeInfo info;
      auto id = static_cast<uint32_t>(jn.at("id").as_int64());
      auto ns = static_cast<uint16_t>(jn.at("ns").as_int64());
      info.node_id = scada::NodeId{id, ns};
      info.name = std::string(jn.at("name").as_string());
      auto cls = jn.at("class").as_string();
      info.node_class = (cls == "variable") ? scada::NodeClass::Variable
                                            : scada::NodeClass::Object;
      if (auto* bv = jn.as_object().if_contains("base_value")) {
        info.base_value = bv->to_number<double>();
        info.has_base_value = true;
      }
      nodes.push_back(std::move(info));
    }
  }

  void ParseTree() {
    for (const auto& [key, val] : json.at("tree").as_object()) {
      std::vector<uint32_t> children;
      for (const auto& child : val.as_array())
        children.push_back(static_cast<uint32_t>(child.as_int64()));
      tree[std::string(key)] = std::move(children);
    }
  }

  void ParseScreenshots() {
    for (const auto& js : json.at("screenshots").as_array()) {
      ScreenshotSpec spec;
      spec.window_type = std::string(js.at("type").as_string());
      spec.filename = std::string(js.at("filename").as_string());
      spec.width = static_cast<int>(js.at("width").as_int64());
      spec.height = static_cast<int>(js.at("height").as_int64());
      screenshots.push_back(std::move(spec));
    }
  }
};

// Global data loaded from JSON.
ScreenshotData g_data;

// --- Helpers ---

std::filesystem::path GetDataFilePath() {
  // Look for screenshot_data.json next to the source file, then in the
  // current working directory.
  for (auto candidate : {
           std::filesystem::path{__FILE__}.parent_path() / "screenshot_data.json",
           std::filesystem::current_path() / "screenshot_data.json",
       }) {
    if (std::filesystem::exists(candidate))
      return candidate;
  }
  return "screenshot_data.json";
}

std::filesystem::path GetOutputDir() {
  if (!g_output_dir.empty())
    return g_output_dir;
  return std::filesystem::current_path() / "screenshots";
}

void SaveScreenshot(QWidget* widget,
                    const ScreenshotData::ScreenshotSpec& spec) {
  if (!widget)
    return;

  widget->resize(spec.width, spec.height);
  // Process pending layout and paint events so the widget renders correctly.
  QApplication::processEvents();
  widget->repaint();
  QApplication::processEvents();

  QPixmap pixmap = widget->grab();
  auto path = GetOutputDir() / spec.filename;
  pixmap.save(QString::fromStdString(path.string()));
}

// --- Graph definition from JSON ---

WindowDefinition MakeGraphDefinition() {
  WindowDefinition def{"Graph"};
  const auto& graph = g_data.json.at("graph").as_object();

  for (const auto& jp : graph.at("panes").as_array()) {
    auto& item = def.AddItem("GraphPane");
    item.SetInt("ix", static_cast<int>(jp.at("index").as_int64()));
    item.SetInt("size", static_cast<int>(jp.at("size").as_int64()));
    if (auto* act = jp.as_object().if_contains("active");
        act && act->as_bool())
      item.SetInt("act", 1);
  }

  for (const auto& ji : graph.at("items").as_array()) {
    def.AddItem("Item")
        .SetString("path", std::string(ji.at("path").as_string()))
        .SetString("clr", std::string(ji.at("color").as_string()))
        .SetInt("pane", static_cast<int>(ji.at("pane").as_int64()))
        .SetInt("dots", ji.at("dots").as_bool() ? 1 : 0)
        .SetInt("stepped", ji.at("stepped").as_bool() ? 1 : 0);
  }

  const auto& ts = graph.at("time_scale").as_object();
  def.AddItem("TimeScale")
      .SetString("time", std::string(ts.at("time").as_string()))
      .SetString("span", std::string(ts.at("span").as_string()))
      .SetBool("scrollBar", ts.at("scroll_bar").as_bool());

  return def;
}

Page MakeScreenshotPage() {
  Page page;
  for (const auto& spec : g_data.screenshots) {
    if (spec.window_type == "Graph")
      page.AddWindow(MakeGraphDefinition());
    else
      page.AddWindow(WindowDefinition{spec.window_type});
  }
  return page;
}

// --- Mock node tree from JSON ---

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

// Persistent storage for mock nodes (must outlive the tree).
struct MockNodeStorage {
  std::map<uint32_t, NodeRef> refs;

  NodeRef Get(const ScreenshotData::NodeInfo& info) {
    auto it = refs.find(info.node_id.numeric_id());
    if (it != refs.end())
      return it->second;
    auto ref = MakeTestNode(info.node_id,
                            UtfConvert<char16_t>(info.name),
                            info.node_class);
    refs[info.node_id.numeric_id()] = ref;
    return ref;
  }
};

static MockNodeStorage g_node_storage;

std::unique_ptr<NodeServiceTree> MakeMockNodeServiceTree(
    NodeServiceTreeImplContext&&) {
  auto tree = std::make_unique<NiceMock<MockNodeServiceTree>>();

  // Build refs for all nodes from JSON.
  for (const auto& info : g_data.nodes)
    g_node_storage.Get(info);

  // Root is the node with id=84, ns=0.
  auto root_info = g_data.FindNode(84);
  assert(root_info);
  auto root = g_node_storage.Get(*root_info);
  ON_CALL(*tree, GetRoot()).WillByDefault(Return(root));

  ON_CALL(*tree, HasChildren(_)).WillByDefault(Return(false));
  ON_CALL(*tree, GetChildren(_))
      .WillByDefault(Return(std::vector<NodeServiceTree::ChildRef>{}));

  auto org = scada::id::Organizes;

  // Set up HasChildren and GetChildren from the tree map.
  // Only entries that have children in the "tree" map (with bare numeric keys
  // for ns=1) are wired up for HasChildren/GetChildren on the NodeServiceTree.
  for (const auto& [key, children] : g_data.tree) {
    // Parse the key to find the node.
    // Keys are "ns.id" (e.g. "0.84") or just "id" (implies ns=1).
    uint32_t id;
    uint16_t ns = 1;
    if (auto dot = key.find('.'); dot != std::string::npos) {
      ns = static_cast<uint16_t>(std::stoi(key.substr(0, dot)));
      id = static_cast<uint32_t>(std::stoi(key.substr(dot + 1)));
    } else {
      id = static_cast<uint32_t>(std::stoi(key));
    }

    // Find the parent node ref.
    auto parent_info = g_data.FindNode(id);
    if (!parent_info)
      continue;
    auto parent_ref = g_node_storage.Get(*parent_info);

    // Build child refs.
    std::vector<NodeServiceTree::ChildRef> child_refs;
    for (uint32_t child_id : children) {
      auto child_info = g_data.FindNode(child_id);
      if (!child_info)
        continue;
      child_refs.push_back({org, true, g_node_storage.Get(*child_info)});
    }

    ON_CALL(*tree, HasChildren(parent_ref)).WillByDefault(Return(true));
    ON_CALL(*tree, GetChildren(parent_ref))
        .WillByDefault(Return(child_refs));
  }

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

scada::BrowseResult MakeBrowseChildren(
    const std::vector<uint32_t>& children) {
  scada::BrowseResult result;
  result.status_code = scada::StatusCode::Good;
  for (uint32_t child_id : children) {
    result.references.push_back(
        {.reference_type_id = scada::NodeId{35, 0},
         .forward = true,
         .node_id = scada::NodeId{child_id, 1}});
  }
  return result;
}

// --- Fake TimedDataService from JSON ---

std::unique_ptr<FakeTimedDataService> MakeFakeTimedDataService() {
  auto service = std::make_unique<FakeTimedDataService>();
  const auto now = base::Time::Now();

  for (const auto& jtd : g_data.json.at("timed_data").as_array()) {
    auto formula = std::string(jtd.at("formula").as_string());
    const auto& jvalues = jtd.at("values").as_array();

    auto td = service->AddTimedData(formula);
    auto count = static_cast<int>(jvalues.size());
    for (int i = 0; i < count; ++i) {
      auto time = now - base::TimeDelta::FromMinutes(30 * (count - i));
      td->data_values.push_back(
          MakeValueAt(scada::Variant{jvalues[i].to_number<double>()}, time));
    }
    td->ready_ranges.push_back(
        {now - base::TimeDelta::FromMinutes(30 * count), now});
  }

  return service;
}

}  // namespace

class ScreenshotGenerator : public Test {
 public:
  static void SetUpTestSuite() {
    g_data.Load(GetDataFilePath());
  }

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
      .node_service_tree_factory_ = MakeMockNodeServiceTree,
      .timed_data_service_override_ = MakeFakeTimedDataService()}};

};

ScreenshotGenerator::ScreenshotGenerator() {
  // Match the default client style and language.
  // Must happen after AppEnvironment creates QApplication but before Start().
  static QSettings settings;
  settings.setValue("LocaleName", "ru");
  static InstalledTranslation installed_translation{settings};
  static InstalledStyle installed_style{settings};

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

  // --- Attribute reads: return display names and values from JSON nodes ---
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
              const auto* node_info = g_data.FindNode(id);
              switch (input.attribute_id) {
                case scada::AttributeId::DisplayName:
                  if (node_info) {
                    results.push_back(
                        MakeValue(scada::Variant{node_info->name}));
                  } else {
                    results.push_back(MakeValue(
                        scada::Variant{"Node " + std::to_string(id)}));
                  }
                  break;

                case scada::AttributeId::Value: {
                  if (node_info && node_info->has_base_value) {
                    static std::mt19937 rng{42};
                    std::normal_distribution<double> dist(
                        node_info->base_value,
                        std::abs(node_info->base_value) * 0.01);
                    results.push_back(MakeValue(scada::Variant{dist(rng)}));
                  } else {
                    results.push_back(MakeValue(scada::Variant{0.0}));
                  }
                  break;
                }

                case scada::AttributeId::NodeClass:
                  if (node_info) {
                    results.push_back(MakeValue(
                        static_cast<scada::Int32>(node_info->node_class)));
                  } else {
                    results.push_back(
                        MakeValue(static_cast<scada::Int32>(1)));
                  }
                  break;

                default:
                  results.push_back(MakeValue(scada::Variant{}));
                  break;
              }
            }
            callback(scada::StatusCode::Good, std::move(results));
          });

  // --- Browse: return children from JSON tree ---
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

              // Try "ns.id" key first, then bare "id" (implies ns=1).
              auto key = std::to_string(ns) + "." + std::to_string(id);
              auto it = g_data.tree.find(key);
              if (it == g_data.tree.end()) {
                key = std::to_string(id);
                it = g_data.tree.find(key);
              }

              if (it != g_data.tree.end()) {
                results.push_back(MakeBrowseChildren(it->second));
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

  // --- History: return time-series data per node ---
  ON_CALL(history_service_, HistoryReadRaw(_, _))
      .WillByDefault(
          [now](const scada::HistoryReadRawDetails& details,
                const scada::HistoryReadRawCallback& callback) {
            auto id = details.node_id.is_numeric()
                          ? details.node_id.numeric_id()
                          : 0u;
            const auto* node_info = g_data.FindNode(id);
            double base_val =
                (node_info && node_info->has_base_value)
                    ? node_info->base_value
                    : 100.0;
            std::mt19937 rng{static_cast<unsigned>(id)};
            std::normal_distribution<double> dist(base_val,
                                                  std::abs(base_val) * 0.05);
            std::vector<scada::DataValue> values;
            for (int i = 0; i < 48; ++i) {
              auto time = now - base::TimeDelta::FromMinutes(30 * (48 - i));
              values.push_back(MakeValueAt(scada::Variant{dist(rng)}, time));
            }
            callback(scada::HistoryReadRawResult{
                .status = scada::StatusCode::Good,
                .values = std::move(values),
            });
          });

  // --- History events from JSON ---
  ON_CALL(history_service_, HistoryReadEvents(_, _, _, _, _))
      .WillByDefault(
          [now](const scada::NodeId&, base::Time, base::Time,
                const scada::EventFilter&,
                const scada::HistoryReadEventsCallback& callback) {
            std::vector<scada::Event> events;

            auto parse_severity = [](std::string_view s) -> scada::UInt32 {
              if (s == "warning")
                return scada::kSeverityWarning;
              if (s == "critical")
                return scada::kSeverityCritical;
              return scada::kSeverityNormal;
            };

            for (const auto& je : g_data.json.at("events").as_array()) {
              scada::Event e;
              e.event_id = static_cast<scada::EventId>(
                  je.at("id").as_int64());
              double hours_ago = je.at("hours_ago").to_number<double>();
              e.time = now - base::TimeDelta::FromSecondsD(hours_ago * 3600);
              e.receive_time = e.time;
              e.severity = parse_severity(
                  je.at("severity").as_string());
              e.message = UtfConvert<char16_t>(
                  std::string(je.at("message").as_string()));
              e.node_id = scada::NodeId{
                  static_cast<uint32_t>(je.at("node_id").as_int64()), 1};
              e.change_mask = static_cast<scada::UInt32>(
                  je.at("change_mask").as_int64());
              e.acked = true;
              e.acknowledged_time = e.time;
              events.push_back(std::move(e));
            }

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
  for (const auto& spec : g_data.screenshots) {
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

  std::cout << "Captured " << captured << "/" << g_data.screenshots.size()
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
