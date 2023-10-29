#include "main_window/main_window.h"

#include "aui/models/simple_menu_model.h"
#include "aui/models/status_bar_model_mock.h"
#include "aui/test/app_environment.h"
#include "base/test/test_executor.h"
#include "filesystem/file_cache.h"
#include "filesystem/file_registry.h"
#include "main_window/action_manager.h"
#include "main_window/controller_factory_mock.h"
#include "main_window/main_window_manager.h"
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

using namespace testing;

class MainWindowTest : public Test {
 protected:
  AppEnvironment app_env_;

  MainWindowContext MakeMainWindowContext();

  const std::shared_ptr<TestExecutor> executor_ =
      std::make_shared<TestExecutor>();
  ActionManager action_manager_;
  FileRegistry file_registry_;
  FileCache file_cache_{file_registry_};
  Profile profile_;
  StrictMock<MockFunction<std::unique_ptr<MainWindow>(int window_id)>>
      main_window_factory_;
  StrictMock<MockFunction<void()>> quit_handler_;
  MockControllerFactory controller_factory_;
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
      .file_registry_ = file_registry_,
      .file_cache_ = file_cache_,
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

TEST_F(MainWindowTest, Test) {
  EXPECT_CALL(quit_handler_, Call());

  main_window_.Close();
}
