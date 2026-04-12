#include "dialog_capture.h"
#include "fixture_builder.h"
#include "graph_capture.h"
#include "screenshot_config.h"
#include "screenshot_output.h"
#include "widget_capture.h"

#include "address_space/local_attribute_service.h"
#include "address_space/local_history_service.h"
#include "address_space/local_monitored_item_service.h"
#include "address_space/local_session_service.h"
#include "address_space/local_view_service.h"
#include "app/client_application.h"
#include "aui/test/app_environment.h"
#include "base/client_paths.h"
#include "base/test/scoped_path_override.h"
#include "base/test/test_executor.h"
#include "configuration/tree/local_node_service_tree.h"
#include "controller/window_info.h"
#include "main_window/main_window.h"
#include "main_window/main_window_manager.h"
#include "main_window/opened_view.h"
#include "node_service/local/local_node_service.h"
#include "profile/profile.h"
#include "timed_data/timed_data_service_fake.h"

#include <QApplication>
#include <QLocale>
#include <QPixmap>
#include <QString>
#include <QTranslator>
#include <boost/asio/io_context.hpp>
#include <gtest/gtest.h>

namespace {

// Global config loaded once per test suite.
ScreenshotConfig g_config;

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

  // Russian translator, installed in the constructor body. Must outlive
  // the QApplication inside `app_env_`, hence declared right after it.
  QTranslator translator_;

  // JSON-backed SCADA services. Populated from `screenshot_data.json`
  // in the constructor body, which runs before any test (and therefore
  // before `app_.Start()`).
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
      .timed_data_service_override_ =
          MakeLocalTimedDataService(g_config.json),
      .node_service_override_ = node_service_}};
};

ScreenshotGenerator::ScreenshotGenerator() {
  // Install the Russian translator directly. We intentionally bypass
  // InstalledTranslation here: it reads the locale from a QSettings
  // instance that lacks an organization/app name (AppEnvironment builds
  // a bare QApplication), so the value round-trips through an
  // unreliable backing store and the Russian .qm ends up not loading
  // at all.
  //
  // The install must happen per-fixture: each TEST_F spins up a fresh
  // QApplication via AppEnvironment, and QTranslator registrations
  // don't survive across QApplication instances.
  QLocale::setDefault(QLocale{QLocale::Russian, QLocale::Russia});
  const auto translation_dir =
      QApplication::applicationDirPath() + "/translations";
  if (translator_.load("client_ru", translation_dir))
    QApplication::installTranslator(&translator_);

  // Match the default client style. Set directly rather than through
  // InstalledStyle for the same QSettings reason: it also writes back
  // the live style's objectName on destruction, which would let the
  // second TEST_F pick up whatever QStyleFactory returned instead of
  // "Fusion".
  QApplication::setStyle("Fusion");

  // Render offscreen.
  MainWindow::SetHideForTesting();

  // Populate the local services from the JSON file.
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

  {
    Profile profile;
    profile.AddPage(MakeScreenshotPage(g_config.screenshots, g_config.json));
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
      // QSplitter children, so we build a fresh graph widget.
      auto timed_data_service = MakeLocalTimedDataService(g_config.json);
      SaveGraphScreenshot(spec, *timed_data_service, g_config.json);
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

  // Size tracks scada-docs/img/client-window.png (1920x1080) so the
  // generator output is a drop-in replacement for the hand-captured
  // doc image.
  auto* qmain = dynamic_cast<const QWidget*>(&main_windows.front());
  if (qmain) {
    auto* mutable_widget = const_cast<QWidget*>(qmain);
    mutable_widget->resize(1920, 1080);
    QApplication::processEvents();
    QPixmap pixmap = mutable_widget->grab();
    auto path = output_dir / "client-window.png";
    pixmap.save(QString::fromStdString(path.string()));
  }
}

TEST_F(ScreenshotGenerator, CaptureDialogs) {
  auto output_dir = GetOutputDir();
  std::filesystem::create_directories(output_dir);

  DialogEnvironment env{.executor = executor_,
                        .node_service = node_service_.get()};

  int captured = 0;
  for (const auto& spec : g_config.dialogs) {
    if (CaptureDialog(spec, env))
      ++captured;
  }

  std::cout << "Captured " << captured << "/" << g_config.dialogs.size()
            << " dialogs to " << output_dir.string() << std::endl;
}

#endif
