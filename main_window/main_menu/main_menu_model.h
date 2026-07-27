#pragma once

#include "base/any_executor.h"

#include "aui/models/simple_menu_model.h"
#include "base/cancelation.h"
#include "controller/command_registry.h"
#include "filesystem/file_cache.h"
#include "main_window/pages/page_switcher.h"

#include <filesystem>
#include <string_view>

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

#endif  // defined(UI_QT)

class MainMenuModel final : private MainMenuContext,
                            private scada::aui::SimpleMenuModel::Delegate,
                            public scada::aui::SimpleMenuModel {
 public:
  explicit MainMenuModel(const MainMenuContext& context);

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
  scada::aui::SimpleMenuModel language_submenu_;
#endif
  scada::aui::SimpleMenuModel settings_submenu_;
  scada::aui::SimpleMenuModel help_submenu_;
};
