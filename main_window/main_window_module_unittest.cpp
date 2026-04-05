#include "main_window/main_window_module.h"

#include "aui/test/app_environment.h"
#include "base/logger.h"
#include "components/web/web_component.h"
#include "controller/controller_fake.h"
#include "controller/test/controller_environment.h"
#include "core/progress_host_impl.h"
#include "events/event_module.h"
#include "favorites/favorites_module.h"
#include "main_window/main_window.h"
#include "main_window/main_window_manager.h"
#include "portfolio/portfolio_module.h"
#include "profile/profile.h"
#include "scada/status_promise.h"
#include "services/speech_service_mock.h"

#include <gmock/gmock.h>

#include "base/debug_util-inl.h"

using namespace testing;

class MainWindowModuleTest : public Test {
 public:
  virtual void SetUp() override;
  virtual void TearDown() override;

 protected:
  AppEnvironment app_env_;
  ControllerEnvironment controller_env_;

  StrictMock<MockFunction<void(const NodeCommandContext& context)>>
      node_command_handler_;

  StrictMock<MockFunction<void()>> quit_handler_;
  StrictMock<MockFunction<void()>> login_handler_;
  StrictMock<MockSpeechService> speech_service_;
  ProgressHostImpl progress_host_;

  StrictMock<MockFunction<std::unique_ptr<
      OpenedView>(MainWindow& main_window, WindowDefinition& window_def)>>
      opened_view_factory_;

  EventModule event_module_{
      {.executor_ = controller_env_.executor_,
       .logger_ = NullLogger::GetInstance(),
       .profile_ = controller_env_.profile_,
       .services_ = controller_env_.services(),
       .controller_registry_ = controller_env_.controller_registry_,
       .selection_commands_ = controller_env_.selection_commands_}};

  PortfolioModule portfolio_module_{
      {.node_service_ = controller_env_.node_service_,
       .profile_ = controller_env_.profile_,
       .controller_registry_ = controller_env_.controller_registry_}};

  FavoritesModule favorites_module{
      {.profile_ = controller_env_.profile_,
       .global_commands_ = controller_env_.global_commands_,
       .controller_registry_ = controller_env_.controller_registry_}};

  std::optional<MainWindowModule> main_window_module_;

  MainWindow* main_window_ = nullptr;

  static const int kWindowId = 111;
};

void MainWindowModuleTest::SetUp() {
  MainWindow::SetHideForTesting();

  ControllerFactory controller_factory = [](unsigned command_id,
                                            ControllerDelegate& delegate,
                                            DialogService& dialog_service) {
    return std::make_unique<FakeController>();
  };

  // Add an empty page so the main window won't try to call `CreateInitialPage`.
  // There are no registered controllers.
  controller_env_.profile_.AddPage({});

  main_window_module_.emplace(MainWindowModuleContext{
      .executor_ = controller_env_.executor_,
      .profile_ = controller_env_.profile_,
      .quit_handler_ = quit_handler_.AsStdFunction(),
      .scada_services_ = controller_env_.services(),
      .login_handler_ = login_handler_.AsStdFunction(),
      .task_manager_ = controller_env_.task_manager_,
      .node_event_provider_ = event_module_.node_event_provider(),
      .timed_data_service_ = controller_env_.timed_data_service_,
      .node_service_ = controller_env_.node_service_,
      .portfolio_manager_ = portfolio_module_.portfolio_manager(),
      .local_events_ = event_module_.local_events(),
      .favourites_ = favorites_module.favourites(),
      .file_cache_ = controller_env_.file_cache_,
      .file_manager_ = controller_env_.file_manager_,
      .speech_service_ = speech_service_,
      .node_command_handler_ = node_command_handler_.AsStdFunction(),
      .progress_host_ = progress_host_,
      .create_tree_ = controller_env_.create_tree_,
      .global_commands_ = controller_env_.global_commands_,
      .selection_commands_ = controller_env_.selection_commands_,
      .controller_factory_ = controller_factory});

  const auto& main_windows =
      main_window_module_->main_window_manager().main_windows();

  ASSERT_THAT(main_windows, SizeIs(1));
  main_window_ = &main_windows.front();
}

void MainWindowModuleTest::TearDown() {
  // Since the module is deleted in non-standard way, we need to clean up the
  // main windows manually.
  for (auto& main_window :
       main_window_module_->main_window_manager().main_windows()) {
    main_window.CleanupForTesting();
  }
}

TEST_F(MainWindowModuleTest, CloseLastWindowInvokesQuitHandler) {
  EXPECT_CALL(quit_handler_, Call());

  main_window_->Close();
}

TEST_F(MainWindowModuleTest, OpensViewWhenDownloadSucceeds) {
  auto path = std::filesystem::path("some/path");

  EXPECT_CALL(controller_env_.file_manager_, DownloadFileFromServer(path));

  auto window_def =
      WindowDefinition{ControllerEnvironment::kFakeWindowInfo}.set_path(path);

  EXPECT_THAT(main_window_->OpenView(window_def).get(), NotNull());
}

TEST_F(MainWindowModuleTest, OpensCachedViewWhenDownloadFails) {
  auto path = std::filesystem::path("some/path");

  EXPECT_CALL(controller_env_.file_manager_, DownloadFileFromServer(path))
      .WillOnce(
          Return(scada::MakeRejectedStatusPromise(scada::StatusCode::Bad)));

  auto window_def =
      WindowDefinition{ControllerEnvironment::kFakeWindowInfo}.set_path(path);

  EXPECT_THAT(main_window_->OpenView(window_def).get(), NotNull());
}

// When the current page is the last not opened, deletes the current page,
// creates another page and switches to it.
// TODO: Fix this test.
TEST_F(MainWindowModuleTest, DISABLED_DeleteCurrentPage_Last) {
  main_window_->DeleteCurrentPage();

  EXPECT_THAT(controller_env_.profile_.pages, SizeIs(1));
  EXPECT_EQ(controller_env_.profile_.pages.begin()->second.GetWindowCount(), 0);
}

// When pages is NOT last, deletes the current page, creates to another page not
// opened page.
TEST_F(MainWindowModuleTest, DeleteCurrentPage_NotLast) {
  int another_page_id = controller_env_.profile_.AddPage(Page{}).id;

  main_window_->DeleteCurrentPage();

  EXPECT_THAT(controller_env_.profile_.pages, SizeIs(1));
  EXPECT_EQ(main_window_->current_page().id, another_page_id);
}