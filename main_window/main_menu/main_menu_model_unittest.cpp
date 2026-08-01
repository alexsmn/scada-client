#include "main_window/main_menu/main_menu_model.h"

#include "aui/dialog_service_mock.h"
#include "aui/models/simple_menu_model.h"
#include "aui/qt/theme_qt.h"
#include "aui/severity_colors.h"
#include "aui/test/app_environment.h"
#include "base/awaitable.h"
#include "base/test/test_executor.h"
#include "controller/command_registry.h"
#include "controller/command_ui_registry.h"
#include "core/global_command_context.h"
#include "favorites/favourites.h"
#include "filesystem/file_cache.h"
#include "filesystem/file_registry.h"
#include "main_window/main_window.h"
#include "main_window/main_window_manager.h"
#include "main_window/main_window_mock.h"
#include "main_window/view_manager.h"
#include "main_window/view_manager_delegate.h"
#include "profile/profile.h"

#include <QApplication>
#include <QMainWindow>
#include <QPalette>
#include <gmock/gmock.h>

#include <optional>
#include <string>
#include <vector>

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
  void OnShowTabPopupMenu(OpenedView& view,
                          const scada::aui::Point& point) override {}
};

// Mirrors the fixture in main_window/pages/page_commands_unittest.cpp: the menu
// models take the whole MainMenuContext, so the cheapest honest way to build
// one is to stand up the real collaborators.
class AppearanceMenuModelTest : public Test {
 protected:
  AppearanceMenuModelTest() {
    // Crossing the Classic↔theme boundary explains that the workbench chrome
    // follows on restart. Give it a real coroutine to await rather than the
    // default-constructed Awaitable a bare NiceMock would return.
    ON_CALL(dialog_service_, RunMessageBox(_, _, _))
        .WillByDefault([](std::u16string_view, std::u16string_view,
                          MessageBoxMode) -> Awaitable<MessageBoxResult> {
          co_return MessageBoxResult::Ok;
        });
  }

  void TearDown() override {
    // The theme is application state; leaving one installed would change every
    // later test in this binary.
    scada::aui::ClearTheme();
  }

  // The model index of the row selecting `theme`, or the "Classic" row when
  // `theme` is unset. Looked up rather than hard-coded so the tests do not
  // silently pass against a reordered menu.
  int IndexOf(std::optional<scada::aui::Theme> theme) {
    for (int i = 0; i < menu_.GetItemCount(); ++i) {
      if (menu_.GetTypeAt(i) != scada::aui::MenuModel::TYPE_RADIO)
        continue;
      if (theme) {
        scada::aui::ApplyTheme(*theme, scada::aui::ThemeScope::kPaletteOnly);
      } else {
        scada::aui::ClearTheme();
      }
      if (menu_.IsItemCheckedAt(i))
        return i;
    }
    return -1;
  }

  AppEnvironment app_env_;
  TestExecutor executor_;
  Profile profile_;

  StrictMock<MockFunction<std::unique_ptr<MainWindow>(int window_id)>>
      main_window_factory_;
  StrictMock<MockFunction<void()>> quit_handler_;
  MainWindowManager main_window_manager_{{profile_,
                                          main_window_factory_.AsStdFunction(),
                                          quit_handler_.AsStdFunction()}};

  StrictMock<MockMainWindow> main_window_;
  NiceMock<MockDialogService> dialog_service_;
  Favourites favourites_;
  FileRegistry file_registry_;
  FileCache file_cache_{file_registry_};
  DummyViewManagerDelegate view_manager_delegate_;
  QMainWindow qt_main_window_;
  ViewManager view_manager_{qt_main_window_, view_manager_delegate_};
  CommandHandler command_handler_;
  scada::aui::SimpleMenuModel context_menu_{nullptr};
  BasicCommandRegistry<GlobalCommandContext> commands_;
  UiCommandRegistry ui_command_registry_;

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
                                .commands_ = commands_,
                                .ui_command_registry_ = ui_command_registry_};

  AppearanceMenuModel menu_{menu_context_};
};

// The menu offers the platform look plus each shipped appearance, as one radio
// group — they are alternatives, not independent toggles.
TEST_F(AppearanceMenuModelTest, OffersClassicAndEveryShippedAppearance) {
  int radio_rows = 0;
  for (int i = 0; i < menu_.GetItemCount(); ++i) {
    if (menu_.GetTypeAt(i) == scada::aui::MenuModel::TYPE_RADIO)
      ++radio_rows;
  }
  EXPECT_EQ(radio_rows, 5);

  // Every appearance is reachable, and Classic is a row of its own rather than
  // an absence of selection.
  EXPECT_NE(IndexOf(std::nullopt), -1);
  for (scada::aui::Theme theme :
       {scada::aui::Theme::kSystem, scada::aui::Theme::kDark,
        scada::aui::Theme::kLight, scada::aui::Theme::kHighContrast}) {
    EXPECT_NE(IndexOf(theme), -1)
        << "appearance " << scada::aui::ThemeToString(theme).toStdString()
        << " has no row";
  }
}

// Choosing a row applies it to the running application — the point of the menu
// is that it behaves like Settings → Style rather than needing a restart.
TEST_F(AppearanceMenuModelTest, ActivatingARowThemesTheLiveApplication) {
  const int dark_row = IndexOf(scada::aui::Theme::kDark);
  ASSERT_NE(dark_row, -1);
  scada::aui::ClearTheme();
  ASSERT_FALSE(scada::aui::IsThemeInstalled());

  menu_.ActivatedAt(dark_row);

  EXPECT_TRUE(scada::aui::IsThemeInstalled());
  EXPECT_EQ(scada::aui::ActiveTheme(), scada::aui::Theme::kDark);
  EXPECT_EQ(qApp->palette().color(QPalette::Window),
            scada::aui::GetThemeTokens(scada::aui::Theme::kDark).bg);
}

// ...and Classic takes it back off, live. Switching off used to be impossible
// without restarting, which is what made the old checkbox ask for one.
TEST_F(AppearanceMenuModelTest, ActivatingClassicRemovesTheThemeLive) {
  const int classic_row = IndexOf(std::nullopt);
  ASSERT_NE(classic_row, -1);
  scada::aui::ApplyTheme(scada::aui::Theme::kHighContrast);
  ASSERT_TRUE(scada::aui::IsThemeInstalled());

  menu_.ActivatedAt(classic_row);

  EXPECT_FALSE(scada::aui::IsThemeInstalled());
  EXPECT_TRUE(qApp->styleSheet().isEmpty());
  EXPECT_EQ(scada::aui::GetSeverityTheme(), scada::aui::SeverityTheme::kLegacy);
}

// The checkmark reports the appearance as *chosen*, not as resolved. While
// following the system, checking "Dark" because the desktop happens to be dark
// would tell the operator they had made a choice they had not made.
TEST_F(AppearanceMenuModelTest, FollowSystemIsCheckedRatherThanItsResolution) {
  const int system_row = IndexOf(scada::aui::Theme::kSystem);
  const int dark_row = IndexOf(scada::aui::Theme::kDark);
  const int light_row = IndexOf(scada::aui::Theme::kLight);
  ASSERT_NE(system_row, -1);

  menu_.ActivatedAt(system_row);

  EXPECT_TRUE(menu_.IsItemCheckedAt(system_row));
  EXPECT_FALSE(menu_.IsItemCheckedAt(dark_row));
  EXPECT_FALSE(menu_.IsItemCheckedAt(light_row));
}

// Exactly one row is checked in every state, including with no theme
// installed — a radio group showing nothing selected reads as broken.
TEST_F(AppearanceMenuModelTest, ExactlyOneRowIsCheckedInEveryState) {
  const std::vector<std::optional<scada::aui::Theme>> kStates = {
      std::nullopt, scada::aui::Theme::kSystem, scada::aui::Theme::kDark,
      scada::aui::Theme::kLight, scada::aui::Theme::kHighContrast};

  for (const auto& state : kStates) {
    if (state) {
      scada::aui::ApplyTheme(*state, scada::aui::ThemeScope::kPaletteOnly);
    } else {
      scada::aui::ClearTheme();
    }
    int checked = 0;
    for (int i = 0; i < menu_.GetItemCount(); ++i) {
      if (menu_.GetTypeAt(i) == scada::aui::MenuModel::TYPE_RADIO &&
          menu_.IsItemCheckedAt(i)) {
        ++checked;
      }
    }
    EXPECT_EQ(checked, 1)
        << "state "
        << (state ? scada::aui::ThemeToString(*state).toStdString()
                  : std::string{"classic"});
  }
}

// The separator between Classic and the token themes must not shift the
// row→appearance mapping: the model indices BuildMenu hands back to
// ActivatedAt() count separators too.
TEST_F(AppearanceMenuModelTest, SeparatorDoesNotShiftTheRowMapping) {
  int separators = 0;
  for (int i = 0; i < menu_.GetItemCount(); ++i) {
    if (menu_.GetTypeAt(i) == scada::aui::MenuModel::TYPE_SEPARATOR)
      ++separators;
  }
  ASSERT_EQ(separators, 1);

  // Activating past the separator still selects the appearance its own row
  // names, and the separator itself is inert.
  for (scada::aui::Theme theme :
       {scada::aui::Theme::kDark, scada::aui::Theme::kLight,
        scada::aui::Theme::kHighContrast}) {
    const int row = IndexOf(theme);
    ASSERT_NE(row, -1);
    scada::aui::ClearTheme();
    menu_.ActivatedAt(row);
    EXPECT_EQ(scada::aui::ActiveTheme(), theme);
  }
}

}  // namespace
