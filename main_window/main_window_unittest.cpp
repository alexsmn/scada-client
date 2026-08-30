#include "main_window/main_window.h"

#include "aui/models/simple_menu_model.h"
#include "aui/models/status_bar_model_impl.h"
#include "aui/models/status_bar_model_mock.h"
#include "aui/test/app_environment.h"
#include "aui/translation.h"
#include "base/test/awaitable_test.h"
#include "base/test/test_executor.h"
#include "controller/action_manager.h"
#include "controller/command_registry.h"
#include "controller/command_ui_registry.h"
#include "controller/controller_factory_mock.h"
#include "controller/controller_mock.h"
#include "controller/test/controller_environment.h"
#include "core/progress_host_impl.h"
#include "export/csv/csv_export_module.h"
#include "main_window/context_menu_model.h"
#include "main_window/main_window_manager.h"
#include "main_window/opened_view/opened_view.h"
#include "main_window/opened_view/opened_view_command_registry.h"
#include "main_window/opened_view/opened_view_command_router.h"
#include "main_window/selection_command_router.h"
#include "modules/graph/graph_component.h"
#include "modules/portfolio/portfolio_module.h"
#include "modules/summary/summary_component.h"
#include "modules/table/table_component.h"
#include "modules/timed_data/timed_data_component.h"
#include "print/service/print_module.h"
#include "profile/page.h"
#include "profile/profile.h"
#include "resources/common_resources.h"
#include "ui/qt/client_utils_qt.h"

#if defined(UI_QT)
#include "main_window/main_window_qt.h"
#endif

#include <gmock/gmock.h>

#if defined(UI_QT)
#include <QAction>
#include <QLabel>
#include <QMenu>
#include <QMenuBar>
#endif

#include "base/debug_util.h"

#include <span>

using namespace testing;
namespace {

Awaitable<void> CompleteDownloadAsync() {
  co_return;
}

class TestMainMenuModel final : public scada::aui::SimpleMenuModel {
 public:
  explicit TestMainMenuModel(std::u16string label = u"Top")
      : scada::aui::SimpleMenuModel{nullptr}, submenu_{nullptr} {
    submenu_.AddItem(1, u"Action");
    AddSubMenu(0, std::move(label), &submenu_);
  }

 private:
  scada::aui::SimpleMenuModel submenu_;
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

class TestOpenedViewCommandRouter final : public CommandHandler {
 public:
  explicit TestOpenedViewCommandRouter(Controller& controller)
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
      std::make_unique<TestOpenedViewCommandRouter>(state->view->controller());

  return state;
}

bool MenuContainsCommand(scada::aui::MenuModel& menu_model,
                         unsigned command_id) {
  auto* model = &menu_model;
  int index = -1;
  return scada::aui::MenuModel::GetModelAndIndexForCommandId(command_id, &model,
                                                             &index);
}

void ExpectMenuContainsCommands(scada::aui::MenuModel& menu_model,
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

  // Production always injects a selection command router (see
  // MainWindowModule); mirror that so activating a view can route selection
  // commands instead of dereferencing a null router.
  std::shared_ptr<SelectionCommandRouter> selection_command_router_ =
      std::make_shared<SelectionCommandRouter>(SelectionCommandRouterContext{
          .selection_commands_ = controller_env_.selection_commands_});

  NiceMock<MockFunction<std::string()>> connection_info_provider_;

  ProgressHostImpl progress_host_;

  std::optional<MainWindow> main_window_;

  static constexpr int kWindowId = 111;
};

MainWindowTest::MainWindowTest() {
  MainWindow::SetHideForTesting();

  // Add an empty page so the main window won't try to call `CreateInitialPage`.
  // There are no registered controllers.
  controller_env_.profile_.AddPage({});

#if defined(UI_QT)
  main_window_.emplace(MakeMainWindowContext());
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
      .main_command_router_factory_ =
          [](MainWindowInterface& main_window, DialogService& dialog_service) {
            return std::make_unique<CommandHandler>();
          },
      .selection_command_router_ = selection_command_router_,
      .status_bar_model_ = std::make_shared<scada::aui::StatusBarModelImpl>(),
      .context_menu_factory_ =
          [](MainWindowInterface& main_window,
             CommandHandler& global_commands) {
            return std::make_unique<scada::aui::SimpleMenuModel>(nullptr);
          },
      .main_menu_factory_ =
          [](MainWindowInterface& main_window, DialogService& dialog_service,
             ViewManager& view_manager, CommandHandler& global_commands,
             scada::aui::MenuModel& context_menu_model) {
            return std::make_unique<scada::aui::SimpleMenuModel>(nullptr);
          },
      .connection_info_provider_ = connection_info_provider_.AsStdFunction(),
      .progress_host_ = progress_host_};
}

MainWindowTest::~MainWindowTest() {
  main_window_->CleanupForTesting();
}

void MainWindowTest::ExpectOpenView() {
  auto controller = std::make_unique<StrictMock<MockController>>();

  // Activating the opened view queries the controller for its selection and
  // contents models (see BaseMainWindow::SetActiveView). These tests don't
  // exercise selection/contents, so mirror the base Controller default of
  // returning none.
  EXPECT_CALL(*controller, GetSelectionModel())
      .Times(AnyNumber())
      .WillRepeatedly(Return(nullptr));
  EXPECT_CALL(*controller, GetContentsModel())
      .Times(AnyNumber())
      .WillRepeatedly(Return(nullptr));

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
// Owns everything a bare MainWindow needs so menu-bar tests can vary just the
// label the main-menu model contributes.
class MainWindowQtHarness {
 public:
  explicit MainWindowQtHarness(std::u16string top_menu_label) {
    MainWindow::SetHideForTesting();
    controller_env_.profile_.AddPage({});
    main_window_.emplace(MainWindowContext{
        .executor_ = controller_env_.executor_,
        .ui_command_registry_ = ui_command_registry_,
        .window_id_ = 111,
        .node_command_handler_ = node_command_handler_.AsStdFunction(),
        .file_manager_ = controller_env_.file_manager_,
        .main_window_manager_ = main_window_manager_,
        .profile_ = controller_env_.profile_,
        .opened_view_factory_ = opened_view_factory_.AsStdFunction(),
        .main_command_router_factory_ =
            [](MainWindowInterface& main_window,
               DialogService& dialog_service) {
              return std::make_unique<CommandHandler>();
            },
        .status_bar_model_ = std::make_shared<scada::aui::StatusBarModelImpl>(),
        .context_menu_factory_ =
            [](MainWindowInterface& main_window,
               CommandHandler& global_commands) {
              return std::make_unique<scada::aui::SimpleMenuModel>(nullptr);
            },
        .main_menu_factory_ =
            [label = std::move(top_menu_label)](
                MainWindowInterface& main_window, DialogService& dialog_service,
                ViewManager& view_manager, CommandHandler& global_commands,
                scada::aui::MenuModel& context_menu_model) {
              return std::make_unique<TestMainMenuModel>(label);
            },
        .connection_info_provider_ = connection_info_provider_.AsStdFunction(),
        .progress_host_ = progress_host_});
  }

  ~MainWindowQtHarness() { main_window_->CleanupForTesting(); }

  MainWindow& main_window() { return *main_window_; }

  // Opens `menu` the way Qt does, so the model-driven contents are built.
  // aboutToShow is a signal, so it cannot be emitted from outside QMenu;
  // invoking it through the meta-object is the supported equivalent.
  static void Show(QMenu& menu) {
    QMetaObject::invokeMethod(&menu, "aboutToShow");
  }

  static std::vector<QString> TopLevelTitles(const MainWindow& main_window) {
    std::vector<QString> titles;
    for (const auto* action : main_window.menuBar()->actions())
      titles.push_back(action->text());
    return titles;
  }

 private:
  AppEnvironment app_env_;
  ControllerEnvironment controller_env_;
  UiCommandRegistry ui_command_registry_;
  StrictMock<MockFunction<void(const NodeCommandContext& context)>>
      node_command_handler_;
  StrictMock<MockFunction<std::unique_ptr<MainWindow>(int window_id)>>
      main_window_factory_;
  StrictMock<MockFunction<void()>> quit_handler_;
  MainWindowManager main_window_manager_{
      {.profile_ = controller_env_.profile_,
       .main_window_factory_ = main_window_factory_.AsStdFunction(),
       .quit_handler_ = quit_handler_.AsStdFunction()}};
  StrictMock<MockFunction<std::unique_ptr<
      OpenedView>(MainWindow& main_window, WindowDefinition& window_def)>>
      opened_view_factory_;
  NiceMock<MockFunction<std::string()>> connection_info_provider_;
  ProgressHostImpl progress_host_;
  std::optional<MainWindow> main_window_;
};

TEST(MainWindowQtTest, MenuBarPopulatesTopLevelMenusImmediately) {
  MainWindowQtHarness harness{u"Top"};

  // Exactly the menus the model contributes. The appearance opt-in used to add
  // a fallback Settings menu here; it now lives in the menu model itself
  // (AppearanceMenuModel), so MainWindow no longer appends anything.
  ASSERT_THAT(harness.main_window().menuBar()->actions(), SizeIs(1));
  auto* top_menu = harness.main_window().menuBar()->actions().front()->menu();
  ASSERT_NE(top_menu, nullptr);
  EXPECT_THAT(top_menu->actions(), SizeIs(1));
}

TEST(MainWindowQtTest, MenuBarDoesNotDuplicateTheModelDrivenSettingsMenu) {
  const QString settings_title =
      QString::fromStdU16String(Translate("Settings"));
  MainWindowQtHarness harness{Translate("Settings")};

  // Regression: the experimental-UX toggle used to be appended as a second
  // top-level Translate("Settings") menu, so the menu bar showed two identical
  // adjacent titles. Removing the fallback removed the whole class of bug, but
  // the menu bar must still show exactly what the model asked for.
  EXPECT_THAT(MainWindowQtHarness::TopLevelTitles(harness.main_window()),
              ElementsAre(settings_title));
}

// Regression: SetWindowFlashing was an empty body, so «Flash Main Window on
// Event» was a live Settings checkbox an operator could tick for nothing
// (backlog 636). What this pins is that the request is acted on and latched.
// It cannot reach the taskbar entry itself: QApplication::alert hands the state
// to QPlatformWindow, Qt exposes no way to read it back, and the harness hides
// the window, so there is no platform window to alert in the first place. The
// call is still made — it is inert here rather than skipped.
TEST(MainWindowQtTest, WindowFlashingFollowsTheRequestedState) {
  MainWindowQtHarness harness{u"Top"};

  EXPECT_FALSE(harness.main_window().IsWindowFlashing());

  harness.main_window().SetWindowFlashing(true);
  EXPECT_TRUE(harness.main_window().IsWindowFlashing());

  // OnEvents calls in on every event dispatch, so the repeat must be harmless.
  harness.main_window().SetWindowFlashing(true);
  EXPECT_TRUE(harness.main_window().IsWindowFlashing());

  harness.main_window().SetWindowFlashing(false);
  EXPECT_FALSE(harness.main_window().IsWindowFlashing());
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
  OpenedViewCommandRegistry opened_view_command_registry;

  GraphModule graph_module{GraphModuleContext{
      .executor_ = controller_env_.executor_,
      .file_cache_ = controller_env_.file_cache_,
      .global_commands_ = controller_env_.global_commands_,
      .selection_commands_ = controller_env_.selection_commands_,
      .ui_command_registry_ = ui_command_registry_}};
  TableModule table_module{TableModuleContext{
      .executor_ = controller_env_.executor_,
      .session_service_ = controller_env_.session_service_,
      .global_commands_ = controller_env_.global_commands_,
      .selection_commands_ = controller_env_.selection_commands_,
      .ui_command_registry_ = ui_command_registry_}};
  SummaryModule summary_module{SummaryModuleContext{
      .executor_ = controller_env_.executor_,
      .selection_commands_ = controller_env_.selection_commands_,
      .ui_command_registry_ = ui_command_registry_}};
  TimedDataModule timed_data_module{TimedDataModuleContext{
      .executor_ = controller_env_.executor_,
      .selection_commands_ = controller_env_.selection_commands_,
      .ui_command_registry_ = ui_command_registry_}};
  PrintModule print_module{PrintModuleContext{
      .ui_command_registry_ = ui_command_registry_,
      .opened_view_commands_ = opened_view_command_registry}};
  CsvExportModule csv_export_module{CsvExportModuleContext{
      .ui_command_registry_ = ui_command_registry_,
      .opened_view_commands_ = opened_view_command_registry}};
  RegisterPortfolioCommandActions(ui_command_registry_);

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
      main_window, ui_command_registry_.command_manager(), global_commands};

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
