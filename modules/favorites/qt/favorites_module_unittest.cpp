#include "favorites/favorites_module.h"

#include "aui/qt/dialog_service_impl_qt.h"
#include "aui/test/app_environment.h"
#include "base/test/awaitable_test.h"
#include "base/test/test_executor.h"
#include "controller/command_registry.h"
#include "controller/command_ui_registry.h"
#include "controller/controller_registry.h"
#include "controller/window_info.h"
#include "core/global_command_context.h"
#include "favorites/favourites.h"
#include "main_window/main_window_mock.h"
#include "main_window/opened_view/opened_view_interface.h"
#include "profile/profile.h"
#include "resources/common_resources.h"
#include "test/fake_opened_view.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <QApplication>
#include <QDialog>

using namespace testing;

namespace {

constexpr std::u16string_view kViewTitle = u"Substation 1";

class FavoritesModuleTest : public Test {
 protected:
  // The Add-to-Favourites command saves the active view under its window
  // title, so the fake has to carry one for the store assertion to have
  // anything to match.
  FavoritesModuleTest() { opened_view_.SetWindowTitle(kViewTitle); }

  // Executes ID_VIEW_ADD_TO_FAVOURITES and lets the spawned coroutine run far
  // enough to put the dialog on screen.
  void ExecuteAddToFavouritesCommand() {
    auto* command = global_commands_.FindCommand(ID_VIEW_ADD_TO_FAVOURITES);
    ASSERT_THAT(command, NotNull());
    command->execute_handler(command_context_);
    Drain(executor_);
    QApplication::processEvents();
  }

  // The modal dialog the command is expected to have opened, or nullptr.
  static QDialog* FindVisibleDialog() {
    for (QWidget* widget : QApplication::topLevelWidgets()) {
      if (!widget->isVisible())
        continue;
      if (auto* dialog = qobject_cast<QDialog*>(widget))
        return dialog;
    }
    return nullptr;
  }

  // The dialog deleteLater's itself on finish; flush the DeferredDelete event
  // so the fixture tears down with no dangling top-level widget.
  void CloseDialog(QDialog& dialog, bool accept) {
    if (accept) {
      dialog.accept();
    } else {
      dialog.reject();
    }
    Drain(executor_);
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
  }

  // Declared first: every widget below it needs a live QApplication.
  AppEnvironment app_env_;
  TestExecutor executor_;
  Profile profile_;
  BasicCommandRegistry<GlobalCommandContext> global_commands_;
  ControllerRegistry controller_registry_;
  UiCommandRegistry ui_command_registry_;
  DialogServiceImplQt dialog_service_;
  NiceMock<MockMainWindow> main_window_;
  FakeOpenedView opened_view_;

  FavoritesModule module_{{.executor_ = executor_,
                           .profile_ = profile_,
                           .global_commands_ = global_commands_,
                           .controller_registry_ = controller_registry_,
                           .ui_command_registry_ = ui_command_registry_}};

  GlobalCommandContext command_context_{.main_window = main_window_,
                                        .dialog_service = dialog_service_};
};

// Regression: the command handler discarded the lazy awaitable returned by
// `ShowAddFavouritesDialog`, so the coroutine never started and
// Window -> Add to Favourites silently did nothing.
TEST_F(FavoritesModuleTest, AddToFavouritesCommandShowsTheDialog) {
  ON_CALL(main_window_, GetActiveView()).WillByDefault(Return(&opened_view_));

  ExecuteAddToFavouritesCommand();

  QDialog* dialog = FindVisibleDialog();
  ASSERT_THAT(dialog, NotNull());

  CloseDialog(*dialog, /*accept=*/false);
}

// The other half of the same regression: a dialog that opens is only useful if
// accepting it reaches the store.
TEST_F(FavoritesModuleTest, AcceptingTheDialogAddsTheFavourite) {
  ON_CALL(main_window_, GetActiveView()).WillByDefault(Return(&opened_view_));

  std::vector<std::u16string> added;
  auto subscription = module_.favourites().SubscribeFavouriteAdded(
      [&added](const Page& folder, const WindowDefinition& win) {
        added.push_back(win.title);
      });

  ExecuteAddToFavouritesCommand();

  QDialog* dialog = FindVisibleDialog();
  ASSERT_THAT(dialog, NotNull());
  CloseDialog(*dialog, /*accept=*/true);

  EXPECT_THAT(added, ElementsAre(std::u16string{kViewTitle}));
}

// The command is not offered without a view to save, so it must not open a
// dialog either.
TEST_F(FavoritesModuleTest, NoDialogWithoutAnActiveView) {
  ON_CALL(main_window_, GetActiveView()).WillByDefault(Return(nullptr));

  ExecuteAddToFavouritesCommand();

  EXPECT_THAT(FindVisibleDialog(), IsNull());
}

}  // namespace
