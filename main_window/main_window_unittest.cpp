#include "main_window/main_window.h"

#include "aui/models/simple_menu_model.h"
#include "aui/models/status_bar_model_mock.h"
#include "aui/qt/client_utils_qt.h"
#include "aui/test/app_environment.h"
#include "base/test/awaitable_test.h"
#include "base/test/test_executor.h"
#include "controller/action_manager.h"
#include "controller/command_registry.h"
#include "controller/command_ui_registry.h"
#include "controller/controller_factory_mock.h"
#include "controller/controller_mock.h"
#include "controller/test/controller_environment.h"
#include "core/progress_host_impl.h"
#include "main_window/actions.h"
#include "main_window/context_menu_model.h"
#include "main_window/main_window_manager.h"
#include "main_window/opened_view/opened_view.h"
#include "main_window/status_bar/status_bar_model_impl.h"
#include "profile/page.h"
#include "profile/profile.h"
#include "resources/common_resources.h"

#if defined(UI_QT)
#include "main_window/main_window_qt.h"
#elif defined(UI_WT)
#include "main_window/main_window_wt.h"
#endif

#include <gmock/gmock.h>

#if defined(UI_QT)
#include <QAction>
#include <QLabel>
#include <QMenu>
#include <QMenuBar>
#elif defined(UI_WT)
#include <wt/WContainerWidget.h>
#endif

#include "base/debug_util.h"

#include <span>

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

class TestActionController final : public Controller {
 public:
  explicit TestActionController(std::vector<unsigned> command_ids) {
    for (unsigned command_id : command_ids) {
      command_registry_.AddCommand(
          Command{command_id}.set_execute_handler([] {}));
    }
  }

  std::unique_ptr<UiView> Init(const WindowDefinition& definition) override {
#if defined(UI_QT)
    return std::make_unique<QWidget>();
#else
    return nullptr;
#endif
  }

  CommandHandler* GetCommandHandler(unsigned command_id) override {
    return command_registry_.GetCommandHandler(command_id);
  }

 private:
  CommandRegistry command_registry_;
};

class TestOpenedViewCommands final : public CommandHandler {
 public:
  explicit TestOpenedViewCommands(Controller& controller)
      : controller_{controller} {}

  CommandHandler* GetCommandHandler(unsigned command_id) override {
    return controller_.GetCommandHandler(command_id);
  }

  bool IsCommandEnabled(unsigned command_id) const override {
    auto* handler =
        const_cast<Controller&>(controller_).GetCommandHandler(command_id);
    return handler && handler->IsCommandEnabled(command_id);
  }

  bool IsCommandChecked(unsigned command_id) const override {
    auto* handler =
        const_cast<Controller&>(controller_).GetCommandHandler(command_id);
    return handler && handler->IsCommandChecked(command_id);
  }

  void ExecuteCommand(unsigned command_id) override {
    auto* handler = controller_.GetCommandHandler(command_id);
    ASSERT_NE(handler, nullptr);
    handler->ExecuteCommand(command_id);
  }

 private:
  Controller& controller_;
};

class TestMainWindowInterface final : public MainWindowInterface {
 public:
  int GetMainWindowId() const override { return 1; }

  const Page& GetCurrentPage() const override { return page_; }
  void OpenPage(const Page& page) override { page_ = page; }
  void SetCurrentPageTitle(std::u16string_view title) override {}
  void SaveCurrentPage() override {}
  void DeleteCurrentPage() override {}

  OpenedViewInterface* GetActiveView() const override { return active_view_; }

  OpenedViewInterface* GetActiveDataView() const override {
    return active_view_;
  }

  void ActivateView(const OpenedViewInterface& view) override {
    active_view_ = const_cast<OpenedViewInterface*>(&view);
  }

  std::vector<OpenedViewInterface*> GetOpenedViews() const override {
    return active_view_ ? std::vector<OpenedViewInterface*>{active_view_}
                        : std::vector<OpenedViewInterface*>{};
  }

  Awaitable<OpenedViewInterface*> OpenView(
      const WindowDefinition& window_definition,
      bool activate = true) override {
    co_return nullptr;
  }

  OpenedViewInterface* FindViewByType(
      std::string_view window_type) const override {
    return nullptr;
  }

  void SplitView(OpenedViewInterface& view, bool vertically) override {}

 private:
  Page page_;
  OpenedViewInterface* active_view_ = nullptr;
};

struct TestOpenedViewState {
  explicit TestOpenedViewState(const WindowInfo& window_info)
      : definition{window_info} {}

  WindowDefinition definition;
  std::unique_ptr<OpenedView> view;
};

std::unique_ptr<TestOpenedViewState> MakeOpenedViewWithCommands(
    AnyExecutor executor,
    DialogService& dialog_service,
    std::vector<unsigned> command_ids) {
  auto state = std::make_unique<TestOpenedViewState>(
      ControllerEnvironment::kFakeWindowInfo);
  auto command_ids_ptr =
      std::make_shared<std::vector<unsigned>>(std::move(command_ids));

  state->view = std::make_unique<OpenedView>(OpenedViewContext{
      .executor_ = executor,
      .window_info_ = ControllerEnvironment::kFakeWindowInfo,
      .window_def_ = state->definition,
      .dialog_service_ = dialog_service,
      .controller_factory_ = [command_ids_ptr](
                                 unsigned command_id,
                                 ControllerDelegate& controller_delegate,
                                 DialogService& dialog_service) {
        return std::make_unique<TestActionController>(*command_ids_ptr);
      }});
  state->view->Init();
  state->view->commands =
      std::make_unique<TestOpenedViewCommands>(state->view->controller());

  return state;
}

bool MenuContainsCommand(aui::MenuModel& menu_model, unsigned command_id) {
  auto* model = &menu_model;
  int index = -1;
  return aui::MenuModel::GetModelAndIndexForCommandId(command_id, &model,
                                                      &index);
}

void ExpectMenuContainsCommands(aui::MenuModel& menu_model,
                                std::span<const unsigned> command_ids) {
  for (unsigned command_id : command_ids) {
    SCOPED_TRACE(command_id);
    EXPECT_TRUE(MenuContainsCommand(menu_model, command_id));
  }
}

#if defined(UI_QT)
bool NativeMenuContainsCommand(const QMenu& menu, unsigned command_id) {
  for (auto* action : menu.actions()) {
    if (action->data().isValid() && action->data().toUInt() == command_id) {
      return true;
    }

    if (auto* submenu = action->menu();
        submenu && NativeMenuContainsCommand(*submenu, command_id)) {
      return true;
    }
  }

  return false;
}

void ExpectNativeMenuContainsCommands(const QMenu& menu,
                                      std::span<const unsigned> command_ids) {
  for (unsigned command_id : command_ids) {
    SCOPED_TRACE(command_id);
    EXPECT_TRUE(NativeMenuContainsCommand(menu, command_id));
  }
}
#endif

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
  StrictMock<MockFunction<std::unique_ptr<OpenedView>(
      MainWindow & main_window, WindowDefinition & window_def)>>
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
TEST_F(MainWindowTest,
       OpenView_DownloadCompletes_ProceedsToOpenedViewNormally) {
  auto window_def =
      WindowDefinition{ControllerEnvironment::kFakeWindowInfo}.set_path(
          "some/path");

  EXPECT_CALL(controller_env_.file_manager_,
              DownloadFileFromServer(window_def.path))
      .WillOnce(
          [](const std::filesystem::path&) { return CompleteDownloadAsync(); });

  ExpectOpenView();

  WaitAwaitable(controller_env_.executor_,
                main_window_->OpenView(window_def, /*make_active=*/true));
}
#endif

#if defined(UI_QT)
TEST_F(MainWindowTest, ContextMenuShowsExpectedActionsForActiveOpenedView) {
  AddGlobalActions(ui_command_registry_.action_manager(),
                   controller_env_.node_service_);

  constexpr unsigned kGraphActions[] = {
      ID_VIEW_LEGEND,       ID_GRAPH_DOTS,  ID_GRAPH_STEPS,
      ID_GRAPH_SCROLL_BAR,  ID_GRAPH_COLOR, ID_GRAPH_SETUP,
      ID_GRAPH_BK_COLOR,    ID_NOW,         ID_GRAPH_ADD_PANE,
      ID_GRAPH_DELETE_PANE,
  };

  constexpr unsigned kTableActions[] = {
      ID_TABLE_CONFIG,
      ID_ADD_ITEMS,
      ID_EXPORT_CSV,
      ID_PRINT,
  };

  auto graph_view = MakeOpenedViewWithCommands(
      controller_env_.executor_, controller_env_.dialog_service_,
      {std::begin(kGraphActions), std::end(kGraphActions)});
  auto table_view = MakeOpenedViewWithCommands(
      controller_env_.executor_, controller_env_.dialog_service_,
      {std::begin(kTableActions), std::end(kTableActions)});

  TestMainWindowInterface main_window;
  CommandHandler global_commands;
  ContextMenuModel context_menu{
      main_window, ui_command_registry_.action_manager(), global_commands};

  main_window.ActivateView(*graph_view->view);

  context_menu.MenuWillShow();
  ExpectMenuContainsCommands(context_menu, kGraphActions);
  EXPECT_FALSE(MenuContainsCommand(context_menu, ID_TABLE_CONFIG));
  QMenu graph_menu;
  BuildMenu(graph_menu, context_menu);
  ExpectNativeMenuContainsCommands(graph_menu, kGraphActions);
  EXPECT_FALSE(NativeMenuContainsCommand(graph_menu, ID_TABLE_CONFIG));

  main_window.ActivateView(*table_view->view);

  context_menu.MenuWillShow();
  ExpectMenuContainsCommands(context_menu, kTableActions);
  EXPECT_FALSE(MenuContainsCommand(context_menu, ID_GRAPH_SETUP));
  QMenu table_menu;
  BuildMenu(table_menu, context_menu);
  ExpectNativeMenuContainsCommands(table_menu, kTableActions);
  EXPECT_FALSE(NativeMenuContainsCommand(table_menu, ID_GRAPH_SETUP));

  main_window.ActivateView(*graph_view->view);

  context_menu.MenuWillShow();
  ExpectMenuContainsCommands(context_menu, kGraphActions);
  EXPECT_FALSE(MenuContainsCommand(context_menu, ID_TABLE_CONFIG));
  QMenu reactivated_graph_menu;
  BuildMenu(reactivated_graph_menu, context_menu);
  ExpectNativeMenuContainsCommands(reactivated_graph_menu, kGraphActions);
  EXPECT_FALSE(
      NativeMenuContainsCommand(reactivated_graph_menu, ID_TABLE_CONFIG));
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
