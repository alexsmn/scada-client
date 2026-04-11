#include "client_application.h"

#include "address_space/local_attribute_service.h"
#include "address_space/local_history_service.h"
#include "address_space/local_monitored_item_service.h"
#include "address_space/local_session_service.h"
#include "address_space/local_view_service.h"
#include "app/qt/installed_style.h"
#include "aui/test/app_environment.h"
#include "base/boost_json_file.h"
#include "base/client_paths.h"
#include "base/test/scoped_path_override.h"
#include "base/test/test_executor.h"
#include "base/time_utils.h"
#include "base/utf_convert.h"
#include "configuration/tree/local_node_service_tree.h"
#include "controller/window_info.h"
#include "main_window/main_window.h"
#include "main_window/main_window_manager.h"
#include "main_window/opened_view.h"
#include "node_service/local/local_node_service.h"
#include "profile/profile.h"
#include "timed_data/timed_data_service_fake.h"

#include "graph/metrix_graph.h"

#include <QApplication>
#include <QLocale>
#include <QPixmap>
#include <QTranslator>
#include <boost/asio/io_context.hpp>
#include <boost/json.hpp>
#include <gtest/gtest.h>

#include <filesystem>

namespace {

// Output directory for screenshots. Override via --screenshot-dir flag.
std::filesystem::path g_output_dir;

// --- JSON data loaded at startup ---
//
// Only the graph/screenshot rendering bits consult this directly; node,
// attribute, view, history data is loaded into the local services.

struct ScreenshotSpec {
  std::string window_type;
  std::string filename;
  int width = 800;
  int height = 600;
};

struct ScreenshotConfig {
  boost::json::value json;
  std::vector<ScreenshotSpec> screenshots;

  void Load(const std::filesystem::path& path) {
    auto opt = ReadBoostJsonFromFile(path);
    ASSERT_TRUE(opt.has_value()) << "Failed to read " << path.string();
    json = std::move(*opt);
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

// Global config loaded once per test suite.
ScreenshotConfig g_config;

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

void SaveScreenshot(QWidget* widget, const ScreenshotSpec& spec) {
  if (!widget)
    return;

  widget->resize(spec.width, spec.height);
  QApplication::processEvents();
  widget->repaint();
  QApplication::processEvents();

  QPixmap pixmap = widget->grab();
  auto path = GetOutputDir() / spec.filename;
  pixmap.save(QString::fromStdString(path.string()));
}

// Render a standalone graph widget populated from JSON timed data.
// This bypasses the hidden main window layout issues by creating a fresh
// Graph shown as a top-level window — matching graph_qt's RenderWidget().
void SaveGraphScreenshot(const ScreenshotSpec& spec,
                         TimedDataService& timed_data_service) {
  MetrixGraph graph{MetrixGraphContext{timed_data_service}};
  const auto& jgraph = g_config.json.at("graph").as_object();

  // Create panes.
  std::map<int, MetrixGraph::MetrixPane*> pane_map;
  for (const auto& jp : jgraph.at("panes").as_array()) {
    auto& pane = graph.NewPane();
    int ix = static_cast<int>(jp.at("index").as_int64());
    pane.size_percent_ = static_cast<int>(jp.at("size").as_int64());
    pane_map[ix] = &pane;
    if (auto* act = jp.as_object().if_contains("active");
        act && act->as_bool())
      graph.SelectPane(&pane);
  }

  // Create lines.
  for (const auto& ji : jgraph.at("items").as_array()) {
    auto path = std::string(ji.at("path").as_string());
    int pane_ix = static_cast<int>(ji.at("pane").as_int64());
    auto* pane = pane_map[pane_ix];
    auto& line = graph.NewLine(path, *pane);
    line.SetColor(QColor(QString::fromStdString(
        std::string(ji.at("color").as_string()))));
    line.set_dots_shown(ji.at("dots").as_bool());
    line.set_stepped(ji.at("stepped").as_bool());
  }

  // Set time range.
  auto now = base::Time::Now();
  // Parse span from "HH:MM:SS" string.
  auto span_str = std::string(jgraph.at("time_scale").at("span").as_string());
  base::TimeDelta span;
  Deserialize(span_str, span);
  double from = (now - span).ToDoubleT();
  double to = now.ToDoubleT();
  graph.horizontal_axis().SetTimeFit(false);
  graph.horizontal_axis().SetRange(
      views::GraphRange{from, to, views::GraphRange::TIME});

  graph.UpdateData();

  // Show legends.
  for (auto* pane : graph.panes())
    static_cast<MetrixGraph::MetrixPane*>(pane)->ShowLegend(true);

  // Render — matching graph_qt's RenderWidget pattern exactly.
  graph.setFixedSize(spec.width, spec.height);
  graph.show();
  QApplication::processEvents();

  QPixmap pixmap = graph.grab();
  auto output_path = GetOutputDir() / spec.filename;
  pixmap.save(QString::fromStdString(output_path.string()));
}

// --- Graph definition from JSON ---

WindowDefinition MakeGraphDefinition() {
  WindowDefinition def{"Graph"};
  const auto& graph = g_config.json.at("graph").as_object();

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
  for (const auto& spec : g_config.screenshots) {
    if (spec.window_type == "Graph")
      page.AddWindow(MakeGraphDefinition());
    else
      page.AddWindow(WindowDefinition{spec.window_type});
  }
  return page;
}

// --- Local TimedDataService populated from JSON ---

std::unique_ptr<FakeTimedDataService> MakeLocalTimedDataService() {
  auto service = std::make_unique<FakeTimedDataService>();
  const auto now = base::Time::Now();

  for (const auto& jtd : g_config.json.at("timed_data").as_array()) {
    auto formula = std::string(jtd.at("formula").as_string());
    const auto& jvalues = jtd.at("values").as_array();

    auto td = service->AddTimedData(formula);
    auto count = static_cast<int>(jvalues.size());
    for (int i = 0; i < count; ++i) {
      auto time = now - base::TimeDelta::FromMinutes(30 * (count - i));
      td->data_values.push_back(
          scada::DataValue{scada::Variant{jvalues[i].to_number<double>()},
                           {},
                           time,
                           time});
    }
    td->ready_ranges.push_back(
        {now - base::TimeDelta::FromMinutes(30 * count), now});
  }

  return service;
}

}  // namespace

class ScreenshotGenerator : public ::testing::Test {
 public:
  static void SetUpTestSuite() { g_config.Load(GetDataFilePath()); }

  ScreenshotGenerator();
  ~ScreenshotGenerator();

 protected:
  boost::asio::io_context io_context_;
  std::shared_ptr<Executor> executor_ = std::make_shared<TestExecutor>();
  AppEnvironment app_env_;

  // Russian translator, installed in the constructor body. Must outlive the
  // QApplication inside `app_env_`, hence declared right after it.
  QTranslator translator_;

  // JSON-backed SCADA services — replace the gmock-based mocks the earlier
  // generator used. Populated from `screenshot_data.json` in the constructor
  // body, which runs before any test (and therefore before `app_.Start()`).
  scada::LocalAttributeService attribute_service_;
  scada::LocalViewService view_service_;
  scada::LocalHistoryService history_service_;
  scada::LocalMonitoredItemService monitored_item_service_;
  scada::LocalSessionService session_service_;

  std::shared_ptr<LocalNodeService> node_service_ =
      std::make_shared<LocalNodeService>();
  std::shared_ptr<LocalNodeServiceTree::SharedData> tree_data_ =
      std::make_shared<LocalNodeServiceTree::SharedData>();

  scada::services services_{
      .attribute_service = &attribute_service_,
      .monitored_item_service = &monitored_item_service_,
      .history_service = &history_service_,
      .view_service = &view_service_,
      .session_service = &session_service_,
  };

  base::ScopedPathOverride private_dir_override_{client::DIR_PRIVATE};

  ClientApplication app_{ClientApplicationContext{
      .io_context_ = io_context_,
      .executor_ = executor_,
      .login_handler_ =
          [this](DataServicesContext&&) {
            return make_resolved_promise(std::optional{
                DataServices::FromUnownedServices(services_)});
          },
      .node_service_tree_factory_ =
          LocalNodeServiceTree::MakeFactory(*node_service_, tree_data_),
      .timed_data_service_override_ = MakeLocalTimedDataService(),
      .node_service_override_ = node_service_}};
};

ScreenshotGenerator::ScreenshotGenerator() {
  // Install the Russian translator directly. We intentionally bypass
  // InstalledTranslation here: it reads the locale from a QSettings instance
  // that lacks an organization/app name (AppEnvironment constructs a bare
  // QApplication), so the value round-trips through an unreliable backing
  // store and the Russian .qm ends up not loading at all.
  //
  // The install must happen per-fixture: each TEST_F spins up a fresh
  // QApplication via AppEnvironment, and QTranslator registrations don't
  // survive across QApplication instances.
  QLocale::setDefault(QLocale{QLocale::Russian, QLocale::Russia});
  const auto translation_dir =
      QApplication::applicationDirPath() + "/translations";
  if (translator_.load("client_ru", translation_dir))
    QApplication::installTranslator(&translator_);

  // Match the default client style.
  static QSettings settings;
  static InstalledStyle installed_style{settings};

  // Don't actually show windows on screen -- render offscreen only.
  MainWindow::SetHideForTesting();

  // Populate the local services from the JSON file. The `app_` member was
  // already constructed above, but it only wires up the services — nothing
  // calls into them until `app_.Start()` runs inside the test body.
  attribute_service_.LoadFromJson(g_config.json);
  view_service_.LoadFromJson(g_config.json);
  history_service_.LoadFromJson(g_config.json);
  node_service_->LoadFromJson(g_config.json);
  tree_data_->LoadFromJson(g_config.json);
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
  ASSERT_EQ(main_windows.size(), 1u);
  const MainWindow& main_window = main_windows.front();

  int captured = 0;
  for (const auto& spec : g_config.screenshots) {
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

    if (spec.window_type == "Graph") {
      // Render graph standalone — hidden main windows don't lay out
      // QSplitter children, so we create a fresh graph widget.
      auto timed_data_service = MakeLocalTimedDataService();
      SaveGraphScreenshot(spec, *timed_data_service);
    } else {
      SaveScreenshot(widget, spec);
    }
    ++captured;
  }

  std::cout << "Captured " << captured << "/" << g_config.screenshots.size()
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
  ASSERT_EQ(main_windows.size(), 1u);

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
