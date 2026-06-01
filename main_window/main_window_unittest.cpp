#include "main_window/main_window.h"

#include "aui/models/simple_menu_model.h"
#include "aui/models/status_bar_model_mock.h"
#include "aui/test/app_environment.h"
#include "base/test/awaitable_test.h"
#include "base/test/test_executor.h"
#include "controller/controller_factory_mock.h"
#include "controller/controller_mock.h"
#include "controller/test/controller_environment.h"
#include "core/progress_host_impl.h"
#include "controller/action_manager.h"
#include "controller/command_ui_registry.h"
#include "main_window/main_window_manager.h"
#include "main_window/opened_view/opened_view.h"
#include "main_window/status_bar/status_bar_model_impl.h"
#include "profile/profile.h"

#if defined(UI_QT)
#include "main_window/main_window_qt.h"
#elif defined(UI_WT)
#include "main_window/main_window_wt.h"
#endif

#include <gmock/gmock.h>

#if defined(UI_QT)
#include <QLabel>
#include <QMenuBar>
#elif defined(UI_WT)
#include <wt/WContainerWidget.h>
#endif

#include "base/debug_util.h"

using namespace testing;
namespace {

Awaitable<void> CompleteDownloadAsync() {
  co_return;
}

class TestMainMenuModel final : public aui::SimpleMenuModel {
 public:
  TestMainMenuModel() : aui::SimpleMenuModel{nullptr}, submenu_{nullptr} {
    submenu_.AddItem(1, u"Action");
    AddSubMenu(0, u"Top", &submenu_);
  }

 private:
  aui::SimpleMenuModel submenu_;
};

}  // namespace

class MainWindowTest : public Test {
 public:
  MainWindowTest();
  ~MainWindowTest();

 protected:
  void ExpectOpenView();

  AppEnvironment app_env_;
  ControllerEnvironment controller_env_;

  MainWindowContext MakeMainWindowContext();

  UiCommandRegistry ui_command_registry_;

  StrictMock<MockFunction<void(const NodeCommandContext& context)>>
      node_command_handler_;

  StrictMock<MockControllerFactory> controller_factory_;

  StrictMock<MockFunction<std::unique_ptr<MainWindow>(int window_id)>>
      main_window_factory_;

  StrictMock<MockFunction<void()>> quit_handler_;

  StrictMock<MockFunction<std::unique_ptr<
      OpenedView>(MainWindow& main_window, WindowDefinition& window_def)>>
      opened_view_factory_;

  MainWindowManager main_window_manager_{
      {.profile_ = controller_env_.profile_,
       .main_window_factory_ = main_window_factory_.AsStdFunction(),
       .quit_handler_ = quit_handler_.AsStdFunction()}};

  NiceMock<MockFunction<std::string()>> connection_info_provider_;

  ProgressHostImpl progress_host_;

#if defined(UI_WT)
  Wt::WContainerWidget container_;
#endif

  std::optional<MainWindow> main_window_;

  static const int kWindowId = 111;
};

MainWindowTest::MainWindowTest() {
  MainWindow::SetHideForTesting();

  // Add an empty page so the main window won't try to call `CreateInitialPage`.
  // There are no registered controllers.
  controller_env_.profile_.AddPage({});

#if defined(UI_QT)
  main_window_.emplace(MakeMainWindowContext());
#elif defined(UI_WT)
  main_window_.emplace(container_, MakeMainWindowContext());
#endif
}

MainWindowContext MainWindowTest::MakeMainWindowContext() {
  return {
      .executor_ = controller_env_.executor_,
      .ui_command_registry_ = ui_command_registry_,
      .window_id_ = kWindowId,
      .node_command_handler_ = node_command_handler_.AsStdFunction(),
      .file_manager_ = controller_env_.file_manager_,
      .main_window_manager_ = main_window_manager_,
      .profile_ = controller_env_.profile_,
      .opened_view_factory_ = opened_view_factory_.AsStdFunction(),
      .main_commands_factory_ =
          [](MainWindowInterface& main_window, DialogService& dialog_service) {
            return std::make_unique<CommandHandler>();
          },
      .status_bar_model_ = std::make_shared<StatusBarModelImpl>(),
      .context_menu_factory_ =
          [](MainWindowInterface& main_window,
             CommandHandler& global_commands) {
            return std::make_unique<aui::SimpleMenuModel>(nullptr);
          },
      .main_menu_factory_ =
          [](MainWindowInterface& main_window, DialogService& dialog_service,
             ViewManager& view_manager, CommandHandler& global_commands,
             aui::MenuModel& context_menu_model) {
            return std::make_unique<aui::SimpleMenuModel>(nullptr);
          },
      .connection_info_provider_ = connection_info_provider_.AsStdFunction(),
      .progress_host_ = progress_host_};
}

MainWindowTest::~MainWindowTest() {
  main_window_->CleanupForTesting();
}

void MainWindowTest::ExpectOpenView() {
  auto controller = std::make_unique<StrictMock<MockController>>();

// TODO: Generalize this test for all UIs.
#if defined(UI_QT)
  EXPECT_CALL(*controller, Init(/*window_def=*/_))
      .WillOnce(Return(ByMove(std::make_unique<QWidget>())));
#endif

  EXPECT_CALL(controller_factory_,
              Call(/*command_id=*/_,
                   /*controller_delegate=*/_, /*dialog_service=*/_))
      .WillOnce(Return(ByMove(std::move(controller))));

  // The passed window definition is copied into the page.
  EXPECT_CALL(opened_view_factory_, Call(/*main_window=*/_,
                                         /*window_def=*/_))
      .WillOnce(
          Invoke([this](MainWindow& main_window, WindowDefinition& window_def) {
            auto opened_view = std::make_unique<OpenedView>(OpenedViewContext{
                .executor_ = controller_env_.executor_,
                .window_info_ = ControllerEnvironment::kFakeWindowInfo,
                .window_def_ = window_def,
                .dialog_service_ = main_window.GetDialogService(),
                .controller_factory_ = controller_factory_.AsStdFunction()});
            opened_view->Init();
            return opened_view;
          }));
}

TEST_F(MainWindowTest, Close_InvokesQuitHandler) {
  EXPECT_CALL(quit_handler_, Call());

  main_window_->Close();
}

#if defined(UI_QT)
TEST(MainWindowQtTest, MenuBarPopulatesTopLevelMenusImmediately) {
  MainWindow::SetHideForTesting();

  AppEnvironment app_env;
  ControllerEnvironment controller_env;
  controller_env.profile_.AddPage({});
  UiCommandRegistry ui_command_registry;
  StrictMock<MockFunction<void(const NodeCommandContext& context)>>
      node_command_handler;
  StrictMock<MockFunction<std::unique_ptr<MainWindow>(int window_id)>>
      main_window_factory;
  StrictMock<MockFunction<void()>> quit_handler;
  MainWindowManager main_window_manager{
      {.profile_ = controller_env.profile_,
       .main_window_factory_ = main_window_factory.AsStdFunction(),
       .quit_handler_ = quit_handler.AsStdFunction()}};
  StrictMock<MockFunction<std::unique_ptr<
      OpenedView>(MainWindow& main_window, WindowDefinition& window_def)>>
      opened_view_factory;
  NiceMock<MockFunction<std::string()>> connection_info_provider;
  ProgressHostImpl progress_host;

  MainWindow main_window{
      {.executor_ = controller_env.executor_,
       .ui_command_registry_ = ui_command_registry,
       .window_id_ = 111,
       .node_command_handler_ = node_command_handler.AsStdFunction(),
       .file_manager_ = controller_env.file_manager_,
       .main_window_manager_ = main_window_manager,
       .profile_ = controller_env.profile_,
       .opened_view_factory_ = opened_view_factory.AsStdFunction(),
       .main_commands_factory_ =
           [](MainWindowInterface& main_window,
              DialogService& dialog_service) {
             return std::make_unique<CommandHandler>();
           },
       .status_bar_model_ = std::make_shared<StatusBarModelImpl>(),
       .context_menu_factory_ =
           [](MainWindowInterface& main_window,
              CommandHandler& global_commands) {
             return std::make_unique<aui::SimpleMenuModel>(nullptr);
           },
       .main_menu_factory_ =
           [](MainWindowInterface& main_window, DialogService& dialog_service,
              ViewManager& view_manager, CommandHandler& global_commands,
              aui::MenuModel& context_menu_model) {
             return std::make_unique<TestMainMenuModel>();
           },
       .connection_info_provider_ = connection_info_provider.AsStdFunction(),
       .progress_host_ = progress_host}};

  ASSERT_THAT(main_window.menuBar()->actions(), SizeIs(1));
  auto* top_menu = main_window.menuBar()->actions().front()->menu();
  ASSERT_NE(top_menu, nullptr);
  EXPECT_THAT(top_menu->actions(), SizeIs(1));

  main_window.CleanupForTesting();
}
#endif

// TODO: Generalize this test for all UIs.
#if defined(UI_QT)
TEST_F(MainWindowTest, OpenView_DownloadSucceeds_OpensViewNormally) {
  auto window_def =
      WindowDefinition{ControllerEnvironment::kFakeWindowInfo}.set_path(
          "some/path");

  EXPECT_CALL(controller_env_.file_manager_,
              DownloadFileFromServer(window_def.path));

  ExpectOpenView();

  WaitAwaitable(controller_env_.executor_,
                main_window_->OpenView(window_def, /*make_active=*/true));
}
#endif

// TODO: Generalize this test for all UIs.
#if defined(UI_QT)
TEST_F(MainWindowTest, OpenView_NoPathSkipsDownloadAndOpensView) {
  auto window_def = WindowDefinition{ControllerEnvironment::kFakeWindowInfo};

  ExpectOpenView();

  WaitAwaitable(controller_env_.executor_,
                main_window_->OpenView(window_def, /*make_active=*/true));
}
#endif

// TODO: Generalize this test for all UIs.
#if defined(UI_QT)
TEST_F(MainWindowTest, OpenView_DownloadCompletes_ProceedsToOpenedViewNormally) {
  auto window_def =
      WindowDefinition{ControllerEnvironment::kFakeWindowInfo}.set_path(
          "some/path");

  EXPECT_CALL(controller_env_.file_manager_,
      DownloadFileFromServer(window_def.path))
      .WillOnce([](const std::filesystem::path&) {
        return CompleteDownloadAsync();
      });

  ExpectOpenView();

  WaitAwaitable(controller_env_.executor_,
                main_window_->OpenView(window_def, /*make_active=*/true));
}
#endif

// When the current page is the last not opened, deletes the current page,
// creates another page and switches to it.
// Disabled because this test fixture intentionally does not register the full
// default controller/window set required by `CreateInitialPage()`.
TEST_F(MainWindowTest, DISABLED_DeleteCurrentPage_Last) {
  main_window_->DeleteCurrentPage();

  EXPECT_THAT(controller_env_.profile_.pages, SizeIs(1));
}

// When pages is NOT last, deletes the current page, creates to another page not
// opened page.
TEST_F(MainWindowTest, DeleteCurrentPage_NotLast) {
  int another_page_id = controller_env_.profile_.AddPage(Page{}).id;

  main_window_->DeleteCurrentPage();

  EXPECT_THAT(controller_env_.profile_.pages, SizeIs(1));
  EXPECT_EQ(main_window_->current_page().id, another_page_id);
}
