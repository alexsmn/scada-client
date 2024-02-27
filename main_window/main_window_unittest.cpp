#include "main_window/main_window.h"

#include "aui/models/simple_menu_model.h"
#include "aui/models/status_bar_model_mock.h"
#include "aui/test/app_environment.h"
#include "base/test/test_executor.h"
#include "controller/controller_factory_mock.h"
#include "controller/controller_mock.h"
#include "filesystem/file_cache.h"
#include "filesystem/file_manager_mock.h"
#include "filesystem/file_registry.h"
#include "main_window/action_manager.h"
#include "main_window/main_window_manager.h"
#include "modus/modus_component.h"
#include "profile/profile.h"

#if defined(UI_QT)
#include "main_window/qt/main_window_qt.h"
#elif defined(UI_WT)
#include "main_window/wt/main_window_wt.h"
#endif

#include <gmock/gmock.h>

#if defined(UI_WT)
#include <wt/WContainerWidget.h>
#endif

#include "base/debug_util-inl.h"

using namespace testing;

class MainWindowTest : public Test {
 public:
  ~MainWindowTest();

 protected:
  void ExpectOpenView(unsigned command_id);

  AppEnvironment app_env_;

  MainWindowContext MakeMainWindowContext();

  const std::shared_ptr<TestExecutor> executor_ =
      std::make_shared<TestExecutor>();

  ActionManager action_manager_;
  FileRegistry file_registry_;

  StrictMock<MockFunction<void(const NodeCommandContext& context)>>
      node_command_handler_;

  FileCache file_cache_{file_registry_};
  StrictMock<MockFileManager> file_manager_;
  Profile profile_;

  StrictMock<MockFunction<std::unique_ptr<MainWindow>(int window_id)>>
      main_window_factory_;

  StrictMock<MockFunction<void()>> quit_handler_;
  StrictMock<MockControllerFactory> controller_factory_;

  MainWindowManager main_window_manager_{
      {.profile_ = profile_,
       .main_window_factory_ = main_window_factory_.AsStdFunction(),
       .quit_handler_ = quit_handler_.AsStdFunction()}};

  std::shared_ptr<aui::MockStatusBarModel> status_bar_model_ =
      std::make_shared<NiceMock<aui::MockStatusBarModel>>();

  NiceMock<MockFunction<std::string()>> connection_info_provider_;

#if defined(UI_QT)
  MainWindowQt main_window_{MakeMainWindowContext()};

#elif defined(UI_WT)
  Wt::WContainerWidget container_;
  MainWindowWt main_window_{container_, MakeMainWindowContext()};
#endif

  static const int kWindowId = 111;
};

MainWindowContext MainWindowTest::MakeMainWindowContext() {
  return {
      .executor_ = executor_,
      .action_manager_ = action_manager_,
      .window_id_ = kWindowId,
      .node_command_handler_ = node_command_handler_.AsStdFunction(),
      .file_cache_ = file_cache_,
      .file_manager_ = file_manager_,
      .main_window_manager_ = main_window_manager_,
      .profile_ = profile_,
      .controller_factory_ = controller_factory_.AsStdFunction(),
      .main_commands_factory_ =
          [](MainWindow& main_window, DialogService& dialog_service) {
            return std::make_unique<CommandHandler>();
          },
      .view_commands_factory_ =
          [](OpenedView& opened_view, DialogService& dialog_service) {
            return std::make_unique<CommandHandler>();
          },
      .status_bar_model_ = status_bar_model_,
      .context_menu_factory_ =
          [](MainWindow& main_window, CommandHandler& main_commands) {
            return std::make_unique<aui::SimpleMenuModel>(nullptr);
          },
      .main_menu_factory_ =
          [](MainWindow& main_window, DialogService& dialog_service,
             ViewManager& view_manager, CommandHandler& main_commands,
             aui::MenuModel& context_menu_model) {
            return std::make_unique<aui::SimpleMenuModel>(nullptr);
          },
      .connection_info_provider_ = connection_info_provider_.AsStdFunction()};
}

MainWindowTest::~MainWindowTest() {
  main_window_.CleanupForTesting();
}

void MainWindowTest::ExpectOpenView(unsigned command_id) {
  MockController* controller = new StrictMock<MockController>{};

  EXPECT_CALL(controller_factory_, Call(command_id,
                                        /*delegate=*/_, /*dialog_service=*/_))
      .WillOnce(Return(std::unique_ptr<MockController>{controller}));

// TODO: Generalize this test for all UIs.
#if defined(UI_QT)
  EXPECT_CALL(*controller, Init(_)).WillOnce(Return(new QWidget));
#endif
}

TEST_F(MainWindowTest, Close_InvokesQuiteHandler) {
  EXPECT_CALL(quit_handler_, Call());

  main_window_.Close();
}

// TODO: Generalize this test for all UIs.
#if defined(UI_QT)
TEST_F(MainWindowTest, OpenView_DowloadSucceeds_OpensViewNormally) {
  auto window_def = WindowDefinition{kModusWindowInfo}.set_path("some/path");

  EXPECT_CALL(file_manager_, DownloadFileFromServer(window_def.path));

  ExpectOpenView(window_def.window_info().command_id);

  main_window_.OpenView(window_def, /*make_active=*/true).get();
}
#endif

// TODO: Generalize this test for all UIs.
#if defined(UI_QT)
TEST_F(MainWindowTest, OpenView_DownloadFails_ProceedsToOpenedViewNormally) {
  auto window_def = WindowDefinition{kModusWindowInfo}.set_path("some/path");

  EXPECT_CALL(file_manager_, DownloadFileFromServer(window_def.path))
      .WillOnce(
          Return(scada::MakeRejectedStatusPromise(scada::StatusCode::Bad)));

  ExpectOpenView(window_def.window_info().command_id);

  main_window_.OpenView(window_def, /*make_active=*/true).get();
}
#endif

// When the current page is the last not opened, deletes the current page,
// creates another page and switches to it.
// TODO: Fix this test.
TEST_F(MainWindowTest, DISABLED_DeleteCurrentPage_Last) {
  main_window_.DeleteCurrentPage();

  EXPECT_THAT(profile_.pages, SizeIs(1));
  EXPECT_EQ(profile_.pages.begin()->second.GetWindowCount(), 0);
}

// When pages is NOT last, deletes the current page, creates to another page not
// opened page.
TEST_F(MainWindowTest, DeleteCurrentPage_NotLast) {
  int another_page_id = profile_.CreatePage().id;

  main_window_.DeleteCurrentPage();

  EXPECT_THAT(profile_.pages, SizeIs(1));
  EXPECT_EQ(main_window_.current_page().id, another_page_id);
}