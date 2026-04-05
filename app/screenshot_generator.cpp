#include "client_application.h"

#include "aui/test/app_environment.h"
#include "controller/window_info.h"
#include "base/client_paths.h"
#include "base/test/scoped_path_override.h"
#include "base/test/test_executor.h"
#include "main_window/main_window.h"
#include "main_window/main_window_manager.h"
#include "main_window/opened_view.h"
#include "profile/profile.h"
#include "scada/services_mock.h"

#include <QApplication>
#include <QPixmap>
#include <boost/asio/io_context.hpp>
#include <gmock/gmock.h>

#include <filesystem>

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

}  // namespace

class ScreenshotGenerator : public Test {
 public:
  ScreenshotGenerator();
  ~ScreenshotGenerator();

 protected:
  boost::asio::io_context io_context_;
  std::shared_ptr<Executor> executor_ = std::make_shared<TestExecutor>();
  AppEnvironment app_env_;
  scada::MockServices services_;

  base::ScopedPathOverride private_dir_override_{client::DIR_PRIVATE};

  NiceMock<MockFunction<promise<std::optional<DataServices>>(
      DataServicesContext&& services_context)>>
      login_handler_;

  ClientApplication app_{ClientApplicationContext{
      .io_context_ = io_context_,
      .executor_ = executor_,
      .login_handler_ = login_handler_.AsStdFunction()}};
};

ScreenshotGenerator::ScreenshotGenerator() {
  // Match the default client style.
  QApplication::setStyle("Fusion");

  // Don't actually show windows on screen -- render offscreen only.
  MainWindow::SetHideForTesting();

  ON_CALL(login_handler_, Call(/*services_context=*/_))
      .WillByDefault(Return(make_resolved_promise(std::optional{
          DataServices::FromUnownedServices(services_.services())})));
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

  // Open a few representative windows.
  {
    Profile profile;
    Page page;
    page.AddWindow(WindowDefinition{"Graph"});
    page.AddWindow(WindowDefinition{"Nodes"});
    page.AddWindow(WindowDefinition{"Event"});
    profile.AddPage(page);
    profile.Save();
  }

  app_.Start().get();

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
