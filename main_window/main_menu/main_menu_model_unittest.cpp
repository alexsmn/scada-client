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
#include "controller/controller_registry.h"
#include "controller/main_menu_window_type_registry.h"
#include "controller/window_info.h"
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
#include "profile/window_definition.h"

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
  void OnShowNewViewMenu(const scada::aui::Point& point) override {}
};

// The appearance menu reads and writes application state only, so it needs no
// MainMenuContext and its fixture stands up nothing but the QApplication.
class AppearanceMenuModelTest : public Test {
 protected:
  void TearDown() override {
    // The theme is application state; leaving an explicitly chosen one behind
    // would change every later test in this binary.
    scada::aui::ApplyTheme(scada::aui::Theme::kSystem,
                           scada::aui::ThemeScope::kPaletteOnly);
  }

  // The model index of the row selecting `theme`. Looked up rather than
  // hard-coded so the tests do not silently pass against a reordered menu.
  int IndexOf(scada::aui::Theme theme) {
    scada::aui::ApplyTheme(theme, scada::aui::ThemeScope::kPaletteOnly);
    for (int i = 0; i < menu_.GetItemCount(); ++i) {
      if (menu_.GetTypeAt(i) != scada::aui::MenuModel::TYPE_RADIO)
        continue;
      if (menu_.IsItemCheckedAt(i))
        return i;
    }
    return -1;
  }

  AppEnvironment app_env_;
  AppearanceMenuModel menu_;
};

// Two display window types, one of which never has a file. Registered through
// a real ControllerRegistry, which is what FindWindowInfoByName() reads.
constexpr WindowInfo kEmptyDisplayInfo = {.command_id = 60001,
                                          .name = "MenuTestEmptyDisplay",
                                          .title = u"Empty display"};
constexpr WindowInfo kFileDisplayInfo = {.command_id = 60002,
                                         .name = "MenuTestFileDisplay",
                                         .title = u"File display"};

// Mirrors the fixture in main_window/pages/page_commands_unittest.cpp: these
// models take the whole MainMenuContext, so the cheapest honest way to build
// one is to stand up the real collaborators.
class MainMenuListsTest : public Test {
 protected:
  MainMenuListsTest() {
    for (const WindowInfo* info : {&kEmptyDisplayInfo, &kFileDisplayInfo}) {
      controller_registry_.AddControllerFactory(
          *info, [](const ControllerContext&) { return nullptr; });
    }
  }

  ~MainMenuListsTest() override {
    // The display-type registry is process state; later tests must not
    // inherit these two types.
    UnregisterDisplayMenuWindowType(kEmptyDisplayInfo.name);
    UnregisterDisplayMenuWindowType(kFileDisplayInfo.name);
  }

  void AddFile(const WindowInfo& info,
               const std::filesystem::path& path,
               const std::u16string& title) {
    FileCache::ItemMap items;
    file_cache_.Update(info.command_id, path, title, items);
  }

  AppEnvironment app_env_;
  TestExecutor executor_;
  Profile profile_;
  ControllerRegistry controller_registry_;

  StrictMock<MockFunction<std::unique_ptr<MainWindow>(int window_id)>>
      main_window_factory_;
  StrictMock<MockFunction<void()>> quit_handler_;
  MainWindowManager main_window_manager_{{profile_,
                                          main_window_factory_.AsStdFunction(),
                                          quit_handler_.AsStdFunction()}};

  NiceMock<MockMainWindow> main_window_;
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
};

// Regression: the "<No displays>" placeholder was added once per window type
// that had no files, so a type listed ahead of a populated one put a
// placeholder row in front of the files, and each file row opened the file one
// below it -- the last one reading past the end of the list.
TEST_F(MainMenuListsTest, AnEmptyDisplayTypeDoesNotShiftTheRowsAfterIt) {
  RegisterDisplayMenuWindowType(kEmptyDisplayInfo.name);
  RegisterDisplayMenuWindowType(kFileDisplayInfo.name);
  AddFile(kFileDisplayInfo, "alpha.sde", u"Alpha");
  AddFile(kFileDisplayInfo, "beta.sde", u"Beta");

  DisplayMenuModel menu{menu_context_};
  menu.MenuWillShow();

  ASSERT_EQ(menu.GetItemCount(), 2);
  EXPECT_EQ(menu.GetLabelAt(0), u"Alpha");
  EXPECT_EQ(menu.GetLabelAt(1), u"Beta");
  EXPECT_TRUE(menu.IsEnabledAt(1));

  EXPECT_CALL(main_window_, OpenView(Field(&WindowDefinition::path,
                                           std::filesystem::path{"beta.sde"}),
                                     true));
  menu.ActivatedAt(1);
  executor_.Poll();
}

// With no files anywhere the menu still says so, exactly once, and the
// placeholder does nothing when activated.
TEST_F(MainMenuListsTest, NoDisplaysShowsOnePlaceholder) {
  RegisterDisplayMenuWindowType(kEmptyDisplayInfo.name);
  RegisterDisplayMenuWindowType(kFileDisplayInfo.name);

  DisplayMenuModel menu{menu_context_};
  menu.MenuWillShow();

  ASSERT_EQ(menu.GetItemCount(), 1);
  EXPECT_FALSE(menu.IsEnabledAt(0));

  EXPECT_CALL(main_window_, OpenView).Times(0);
  menu.ActivatedAt(0);
  executor_.Poll();
}

// Regression: a trashed window whose type is no longer registered gets no row,
// but activation indexed the trash by row, so "Restore <File display>" reopened
// the unregistered window instead and dropped it from the trash.
TEST_F(MainMenuListsTest, TrashRestoresTheWindowItsRowNames) {
  profile_.trash.AddWindow(WindowDefinition{"MenuTestUnregistered"});
  WindowDefinition registered{kFileDisplayInfo};
  registered.title = u"Kept";
  profile_.trash.AddWindow(registered);

  TrashMenuModel menu{menu_context_};
  menu.MenuWillShow();
  ASSERT_EQ(menu.GetItemCount(), 1);
  ASSERT_TRUE(menu.IsEnabledAt(0));

  EXPECT_CALL(main_window_, OpenView(Field(&WindowDefinition::type,
                                           std::string{kFileDisplayInfo.name}),
                                     true));
  menu.ActivatedAt(0);
  executor_.Poll();

  ASSERT_EQ(profile_.trash.GetWindowCount(), 1);
  EXPECT_EQ(profile_.trash.GetWindow(0).type, "MenuTestUnregistered");
}

// The menu offers each shipped appearance as one radio group — they are
// alternatives, not independent toggles. There is no sixth "Classic" row: the
// un-themed platform look was removed on 2026-08-31, and with it the only state
// in which no appearance was installed.
TEST_F(AppearanceMenuModelTest, OffersEveryShippedAppearanceAndNothingElse) {
  int radio_rows = 0;
  for (int i = 0; i < menu_.GetItemCount(); ++i) {
    if (menu_.GetTypeAt(i) == scada::aui::MenuModel::TYPE_RADIO)
      ++radio_rows;
  }
  EXPECT_EQ(radio_rows, 4);

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
  scada::aui::ApplyTheme(scada::aui::Theme::kLight,
                         scada::aui::ThemeScope::kPaletteOnly);

  menu_.ActivatedAt(dark_row);

  EXPECT_EQ(scada::aui::ActiveTheme(), scada::aui::Theme::kDark);
  EXPECT_EQ(qApp->palette().color(QPalette::Window),
            scada::aui::GetThemeTokens(scada::aui::Theme::kDark).bg);
  // The severity ramp follows, so the process-semantic colours cannot disagree
  // with the chrome they sit on.
  EXPECT_EQ(scada::aui::GetSeverityTheme(), scada::aui::SeverityTheme::kDark);
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

// Exactly one row is checked in every state — a radio group showing nothing
// selected reads as broken.
TEST_F(AppearanceMenuModelTest, ExactlyOneRowIsCheckedInEveryState) {
  for (const scada::aui::Theme state :
       {scada::aui::Theme::kSystem, scada::aui::Theme::kDark,
        scada::aui::Theme::kLight, scada::aui::Theme::kHighContrast}) {
    scada::aui::ApplyTheme(state, scada::aui::ThemeScope::kPaletteOnly);
    int checked = 0;
    for (int i = 0; i < menu_.GetItemCount(); ++i) {
      if (menu_.GetTypeAt(i) == scada::aui::MenuModel::TYPE_RADIO &&
          menu_.IsItemCheckedAt(i)) {
        ++checked;
      }
    }
    EXPECT_EQ(checked, 1) << "state "
                          << scada::aui::ThemeToString(state).toStdString();
  }
}

// Every row is selectable and selects the appearance its own row names. The
// menu carried a separator — between "Classic" and the themes — until
// 2026-08-31, and the row→appearance mapping had to survive it because the
// model indices BuildMenu hands back to ActivatedAt() count separators too.
// It carries none now, which is exactly why this asserts on the mapping rather
// than on the separator: a row that stops naming its own appearance is the
// defect, whether or not anything sits between the rows.
TEST_F(AppearanceMenuModelTest, EveryRowSelectsTheAppearanceItNames) {
  for (const scada::aui::Theme theme :
       {scada::aui::Theme::kSystem, scada::aui::Theme::kDark,
        scada::aui::Theme::kLight, scada::aui::Theme::kHighContrast}) {
    const int row = IndexOf(theme);
    ASSERT_NE(row, -1);
    // Move somewhere else first, so a row that selects nothing at all cannot
    // pass by leaving the appearance IndexOf() just installed in place.
    scada::aui::ApplyTheme(theme == scada::aui::Theme::kLight
                               ? scada::aui::Theme::kDark
                               : scada::aui::Theme::kLight,
                           scada::aui::ThemeScope::kPaletteOnly);
    menu_.ActivatedAt(row);
    EXPECT_EQ(scada::aui::ActiveTheme(), theme);
  }
}

}  // namespace
