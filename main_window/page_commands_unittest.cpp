#include "page_commands.h"

#include "aui/dialog_service_mock.h"
#include "aui/models/simple_menu_model.h"
#include "base/test/awaitable_test.h"
#include "base/test/test_executor.h"
#include "controller/command_registry.h"
#include "core/global_command_context.h"
#include "favorites/favourites.h"
#include "filesystem/file_cache.h"
#include "filesystem/file_registry.h"
#include "main_window/main_menu_model.h"
#include "main_window/main_window.h"
#include "main_window/main_window_manager.h"
#include "main_window/main_window_mock.h"
#include "main_window/view_manager.h"
#include "main_window/view_manager_delegate.h"
#include "profile/profile.h"

#include <gmock/gmock.h>

#include <chrono>

using namespace testing;

namespace {

class DummyViewManagerDelegate : public ViewManagerDelegate {
 public:
  std::unique_ptr<OpenedView> OnCreateView(
      WindowDefinition& definition) override {
    return nullptr;
  }
  void OnViewClosed(OpenedView& view) override {}
  void OnActiveViewChanged(OpenedView* view) override {}
  void OnShowTabPopupMenu(OpenedView& view, const aui::Point& point) override {}
};

class DummyViewManager : public ViewManager {
 public:
  explicit DummyViewManager(ViewManagerDelegate& delegate)
      : ViewManager{delegate} {}

  OpenedView* GetActiveView() override { return nullptr; }
  void ActivateView(const OpenedView& view) override {}
  void CloseView(OpenedView& view) override {}
  void SetViewTitle(OpenedView& view, const std::u16string& title) override {}
  void SplitView(OpenedView& view, bool vertically) override {}

 private:
  void OpenLayout(Page& page, const PageLayout& layout) override {}
  void SaveLayout(PageLayout& layout) override {}
  void AddView(OpenedView& view) override {}
};

}  // namespace

class PageCommandsTest : public Test {
 protected:
  std::shared_ptr<TestExecutor> executor_ = std::make_shared<TestExecutor>();
  BasicCommandRegistry<GlobalCommandContext> commands_;
  Profile profile_;

  StrictMock<MockFunction<std::unique_ptr<MainWindow>(int window_id)>>
      main_window_factory_;

  StrictMock<MockFunction<void()>> quit_handler_;

  MainWindowManager main_window_manager_{{profile_,
                                          main_window_factory_.AsStdFunction(),
                                          quit_handler_.AsStdFunction()}};

  StrictMock<MockMainWindow> main_window_;
  StrictMock<MockDialogService> dialog_service_;
  StrictMock<MockFunction<promise<std::u16string>(
      DialogService& dialog_service,
      std::u16string current_title)>>
      rename_prompt_runner_;

  GlobalCommandContext command_context_{.main_window = main_window_,
                                      .dialog_service = dialog_service_};

  PageCommands page_commands_{
      {executor_, commands_, profile_, main_window_manager_,
       rename_prompt_runner_.AsStdFunction()}};
};

TEST_F(PageCommandsTest, DeletePage) {
  auto* command = commands_.FindCommand(ID_PAGE_DELETE);
  ASSERT_THAT(command, NotNull());

  EXPECT_CALL(main_window_, DeleteCurrentPage());

  command->execute_handler(command_context_);
}

TEST_F(PageCommandsTest, RenamePageAcceptedUpdatesCurrentPageTitle) {
  Page page;
  page.title = u"Old title";

  auto* command = commands_.FindCommand(ID_PAGE_RENAME);
  ASSERT_THAT(command, NotNull());

  EXPECT_CALL(main_window_, GetCurrentPage()).WillOnce(ReturnRef(page));
  EXPECT_CALL(rename_prompt_runner_,
              Call(Ref(dialog_service_), std::u16string{u"Old title"}))
      .WillOnce(Return(make_resolved_promise(std::u16string{u"New title"})));

  promise<void> renamed;
  EXPECT_CALL(main_window_, SetCurrentPageTitle(std::u16string_view{u"New title"}))
      .WillOnce([&renamed](std::u16string_view) mutable { renamed.resolve(); });

  command->execute_handler(command_context_);
  Drain(executor_);

  ASSERT_NE(renamed.wait_for(std::chrono::milliseconds{0}),
            promise_wait_status::timeout);
  EXPECT_NO_THROW(renamed.get());
}

TEST_F(PageCommandsTest, RenamePageRejectedDoesNotUpdateCurrentPageTitle) {
  Page page;
  page.title = u"Old title";

  auto* command = commands_.FindCommand(ID_PAGE_RENAME);
  ASSERT_THAT(command, NotNull());

  EXPECT_CALL(main_window_, GetCurrentPage()).WillOnce(ReturnRef(page));
  EXPECT_CALL(rename_prompt_runner_,
              Call(Ref(dialog_service_), std::u16string{u"Old title"}))
      .WillOnce(Return(make_rejected_promise<std::u16string>(std::exception{})));
  EXPECT_CALL(main_window_, SetCurrentPageTitle(_)).Times(0);

  command->execute_handler(command_context_);
  Drain(executor_);
}

class PageMenuModelTest : public Test {
 protected:
  std::shared_ptr<TestExecutor> executor_ = std::make_shared<TestExecutor>();
  Profile profile_;

  StrictMock<MockFunction<std::unique_ptr<MainWindow>(int window_id)>>
      main_window_factory_;
  StrictMock<MockFunction<void()>> quit_handler_;
  MainWindowManager main_window_manager_{{profile_,
                                          main_window_factory_.AsStdFunction(),
                                          quit_handler_.AsStdFunction()}};

  StrictMock<MockMainWindow> main_window_;
  StrictMock<MockDialogService> dialog_service_;
  Favourites favourites_;
  FileRegistry file_registry_;
  FileCache file_cache_{file_registry_};
  DummyViewManagerDelegate view_manager_delegate_;
  DummyViewManager view_manager_{view_manager_delegate_};
  CommandHandler command_handler_;
  aui::SimpleMenuModel context_menu_{nullptr};
  BasicCommandRegistry<GlobalCommandContext> commands_;

  MainMenuContext menu_context_{.executor_ = executor_,
                                .main_window_manager_ = main_window_manager_,
                                .main_window_ = main_window_,
                                .favourites_ = favourites_,
                                .file_cache_ = file_cache_,
                                .admin_ = false,
                                .profile_ = profile_,
                                .view_manager_ = view_manager_,
                                .command_handler_ = command_handler_,
                                .dialog_service_ = dialog_service_,
                                .context_menu_model_ = context_menu_,
                                .commands_ = commands_};
};

TEST_F(PageMenuModelTest, RevertCurrentPageConfirmedOpensSavedPage) {
  Page page_def;
  page_def.title = u"Saved";
  Page& page = profile_.AddPage(page_def);

  EXPECT_CALL(main_window_, GetCurrentPage())
      .WillRepeatedly(ReturnRef(page));
  EXPECT_CALL(dialog_service_,
              RunMessageBox(/*message=*/_, /*title=*/_,
                            MessageBoxMode::QuestionYesNo))
      .WillOnce(Return(make_resolved_promise(MessageBoxResult::Yes)));
  EXPECT_CALL(main_window_, OpenPage(Ref(page)));

  PageMenuModel menu{menu_context_};
  menu.MenuWillShow();
  menu.ActivatedAt(0);
  Drain(executor_);
}

TEST_F(PageMenuModelTest, RevertCurrentPageCanceledDoesNotOpenPage) {
  Page page_def;
  page_def.title = u"Saved";
  Page& page = profile_.AddPage(page_def);

  EXPECT_CALL(main_window_, GetCurrentPage())
      .WillRepeatedly(ReturnRef(page));
  EXPECT_CALL(dialog_service_,
              RunMessageBox(/*message=*/_, /*title=*/_,
                            MessageBoxMode::QuestionYesNo))
      .WillOnce(Return(make_resolved_promise(MessageBoxResult::No)));

  PageMenuModel menu{menu_context_};
  menu.MenuWillShow();
  menu.ActivatedAt(0);
  Drain(executor_);
}
