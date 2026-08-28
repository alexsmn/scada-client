#pragma once

#include "base/any_executor.h"

#include "aui/models/simple_menu_model.h"
#include "base/cancelation.h"
#include "controller/command_registry.h"
#include "filesystem/file_cache.h"
#include "main_window/pages/page_switcher.h"

#include <filesystem>
#include <string_view>
#include <vector>

#if defined(UI_QT)
#include "aui/qt/theme_qt.h"
#endif

class CommandHandler;
class DialogService;
class Favourites;
class MainWindowInterface;
class MainWindowManager;
class Page;
class Profile;
class UiCommandRegistry;
class ViewManager;
class WindowDefinition;
enum class MainMenuId;
struct GlobalCommandContext;
struct WindowInfo;

struct MainMenuContext {
  const AnyExecutor executor_;
  MainWindowManager& main_window_manager_;
  MainWindowInterface& main_window_;
  Favourites& favourites_;
  FileCache& file_cache_;
  const bool admin_;
  Profile& profile_;
  ViewManager& view_manager_;
  CommandHandler& command_handler_;
  DialogService& dialog_service_;
  scada::aui::MenuModel& context_menu_model_;
  BasicCommandRegistry<GlobalCommandContext>& commands_;
  UiCommandRegistry& ui_command_registry_;
};

class DisplayMenuModel : private MainMenuContext,
                         public scada::aui::SimpleMenuModel {
 public:
  explicit DisplayMenuModel(const MainMenuContext& context);

  // views::MenuModel
  virtual void MenuWillShow() override;
  virtual void ActivatedAt(int index) override;
  virtual bool IsEnabledAt(int index) const override;

 private:
  void AddItems(const WindowInfo& window_info);

  struct Item {
    const WindowInfo* window_info = nullptr;
    std::filesystem::path path;
  };

  std::vector<Item> items_;
};

class FavouritesMenuModel : private MainMenuContext,
                            public scada::aui::SimpleMenuModel {
 public:
  FavouritesMenuModel(MainMenuId menu_id, const MainMenuContext& context);

  // views::MenuModel
  virtual void MenuWillShow() override;
  virtual void ActivatedAt(int index) override;
  virtual bool IsEnabledAt(int index) const override;

 private:
  const MainMenuId menu_id_;

  std::vector<const WindowDefinition*> windows_;
};

class PageMenuModel : private MainMenuContext,
                      public scada::aui::SimpleMenuModel {
 public:
  explicit PageMenuModel(const MainMenuContext& context);

  // views::MenuModel
  virtual void MenuWillShow() override;
  virtual void ActivatedAt(int index) override;
  virtual bool IsItemCheckedAt(int index) const override;

 private:
  // The page list and the switching policy, shared with the activity rail's
  // Pages pane so both surfaces behave identically.
  PageSwitcher page_switcher_;

  // The list as of the last MenuWillShow(), so ActivatedAt() resolves an index
  // to a page id without re-walking the profile.
  std::vector<PageEntry> entries_;
  int active_index_ = -1;
};

class WindowMenuModel : private MainMenuContext,
                        public scada::aui::SimpleMenuModel {
 public:
  explicit WindowMenuModel(const MainMenuContext& context)
      : MainMenuContext{context}, scada::aui::SimpleMenuModel{nullptr} {}

  // views::MenuModel
  virtual void MenuWillShow() override;
  virtual void ActivatedAt(int index) override;
  virtual bool IsItemCheckedAt(int index) const override;

 private:
  int active_index_ = -1;
};

class TrashMenuModel : private MainMenuContext,
                       public scada::aui::SimpleMenuModel {
 public:
  explicit TrashMenuModel(const MainMenuContext& context)
      : MainMenuContext{context}, scada::aui::SimpleMenuModel{nullptr} {}

  // views::MenuModel
  virtual void MenuWillShow() override;
  virtual void ActivatedAt(int index) override;
  virtual bool IsEnabledAt(int index) const override;

 private:
  bool empty_ = true;
};

#if defined(UI_QT)
class StyleMenuModel : public scada::aui::SimpleMenuModel {
 public:
  StyleMenuModel();

  // views::MenuModel
  virtual void ActivatedAt(int index) override;
  virtual bool IsItemCheckedAt(int index) const override;
};

// Settings → Colour scheme: the experimental UX design-token themes, offered
// the same way Settings → Style offers widget styles — radio rows that take
// effect live, with no dialog and no restart.
//
// Deliberately a sibling of the Style menu rather than extra rows inside it.
// The reshell is not a QStyle: it is a QPalette (plus a near-empty stylesheet)
// layered *over* whichever platform style is active, so the two are orthogonal
// and listing them together would falsely present them as alternatives.
//
// Checked state comes from the theme module, not from QSettings: the setting
// records what the client started with, the module what the operator is
// actually looking at, and only the second is right for a checkmark once the
// menu can switch it mid-session. Persistence is InstalledAppearance's job
// (app/qt), matching how StyleMenuModel leaves the `Style` key to
// InstalledStyle.
//
// Colour switches live and completely. Crossing between Classic and a theme
// does not: MainWindow builds the reshell chrome (activity bar, context bar,
// Inspector and the specialist docks) once, in its constructor, gated on the
// active theme — so that half of the change lands on restart and the menu says
// so rather than leaving the operator to notice.
class AppearanceMenuModel : private MainMenuContext,
                            public scada::aui::SimpleMenuModel {
 public:
  explicit AppearanceMenuModel(const MainMenuContext& context);

  // views::MenuModel
  virtual void ActivatedAt(int index) override;
  virtual bool IsItemCheckedAt(int index) const override;

 private:
  // Explains that the workbench chrome follows on restart. Shown only when the
  // Classic↔theme boundary is crossed — a switch between two themes is fully
  // live, and prompting there would be noise.
  void NotifyShellFollowsOnRestart();

  // What a row selects, held parallel to the model's items so the separator
  // occupies an index here too and cannot shift the mapping. Keeping the
  // meaning out of the label also keeps it independent of translation.
  struct Row {
    enum class Kind {
      kSeparator,
      kClassic,  // no theme installed: the untouched platform look
      kTheme,
    };

    Kind kind = Kind::kSeparator;
    scada::aui::Theme theme = scada::aui::Theme::kSystem;
  };

  std::vector<Row> rows_;
};

#endif  // defined(UI_QT)

class MainMenuModel final : private MainMenuContext,
                            private scada::aui::SimpleMenuModel::Delegate,
                            public scada::aui::SimpleMenuModel {
 public:
  explicit MainMenuModel(const MainMenuContext& context);

#if defined(UI_QT)
  // The Settings items, which the Qt shell renders as a full-window surface
  // rather than a submenu (see `main_window/settings_view_qt.h`). Still
  // assembled here, from the command registry and every module's contribution,
  // so the surface and the menu fallback are built from one description --
  // `settings_catalog.cpp` joins its rows to exactly these items.
  scada::aui::MenuModel& settings_model() { return settings_submenu_; }
#endif

 private:
  void Rebuild();

  // aui::SimpleMenuModel::Delegate
  virtual bool IsCommandIdChecked(int command_id) const override;
  virtual bool IsCommandIdEnabled(int command_id) const override;
  virtual void ExecuteCommand(int command_id) override;

  DisplayMenuModel display_menu_model_;
  FavouritesMenuModel table_favourites_;
  scada::aui::SimpleMenuModel table_submenu_;
  std::unique_ptr<FavouritesMenuModel> graph_favourites_;
  scada::aui::SimpleMenuModel graph_submenu_;
  scada::aui::SimpleMenuModel more_submenu_;
  PageMenuModel page_list_menu_;
  scada::aui::SimpleMenuModel page_submenu_;
  WindowMenuModel window_list_menu_;
  TrashMenuModel trash_menu_;
  scada::aui::SimpleMenuModel window_submenu_;
#if defined(UI_QT)
  StyleMenuModel style_submenu_;
  AppearanceMenuModel appearance_submenu_;
  scada::aui::SimpleMenuModel language_submenu_;
  // What the menu bar actually mounts under Qt: one item opening the
  // preferences dialog. The toggles themselves live in settings_submenu_,
  // which the dialog renders.
  scada::aui::SimpleMenuModel settings_menu_;
#endif
  scada::aui::SimpleMenuModel settings_submenu_;
  scada::aui::SimpleMenuModel help_submenu_;
};
