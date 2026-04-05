#pragma once

#include "aui/models/simple_menu_model.h"
#include "base/cancelation.h"
#include "controller/command_registry.h"
#include "filesystem/file_cache.h"

#include <filesystem>
#include <span>

class CommandHandler;
class DialogService;
class Executor;
class Favourites;
class MainWindowInterface;
class MainWindowManager;
class Page;
class Profile;
class ViewManager;
class WindowDefinition;
struct GlobalCommandContext;
struct WindowInfo;

struct MainMenuContext {
  const std::shared_ptr<Executor> executor_;
  MainWindowManager& main_window_manager_;
  MainWindowInterface& main_window_;
  Favourites& favourites_;
  FileCache& file_cache_;
  const bool admin_;
  Profile& profile_;
  ViewManager& view_manager_;
  CommandHandler& command_handler_;
  DialogService& dialog_service_;
  aui::MenuModel& context_menu_model_;
  BasicCommandRegistry<GlobalCommandContext>& commands_;
};

class DisplayMenuModel : private MainMenuContext, public aui::SimpleMenuModel {
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
                            public aui::SimpleMenuModel {
 public:
  FavouritesMenuModel(std::span<const WindowInfo* const> window_infos,
                      const MainMenuContext& context);

  // views::MenuModel
  virtual void MenuWillShow() override;
  virtual void ActivatedAt(int index) override;
  virtual bool IsEnabledAt(int index) const override;

 private:
  const std::span<const WindowInfo* const> window_infos_;

  std::vector<const WindowDefinition*> windows_;
};

class PageMenuModel : private MainMenuContext, public aui::SimpleMenuModel {
 public:
  explicit PageMenuModel(const MainMenuContext& context);

  // views::MenuModel
  virtual void MenuWillShow() override;
  virtual void ActivatedAt(int index) override;
  virtual bool IsItemCheckedAt(int index) const override;

 private:
  void OpenPage(const Page& page);
  void OpenPageHelper(const Page& page, bool revert);

  int active_index_ = -1;

  Cancelation cancelation_;
};

class WindowMenuModel : private MainMenuContext, public aui::SimpleMenuModel {
 public:
  explicit WindowMenuModel(const MainMenuContext& context)
      : MainMenuContext{context}, aui::SimpleMenuModel{nullptr} {}

  // views::MenuModel
  virtual void MenuWillShow() override;
  virtual void ActivatedAt(int index) override;
  virtual bool IsItemCheckedAt(int index) const override;

 private:
  int active_index_ = -1;
};

class TrashMenuModel : private MainMenuContext, public aui::SimpleMenuModel {
 public:
  explicit TrashMenuModel(const MainMenuContext& context)
      : MainMenuContext{context}, aui::SimpleMenuModel{nullptr} {}

  // views::MenuModel
  virtual void MenuWillShow() override;
  virtual void ActivatedAt(int index) override;
  virtual bool IsEnabledAt(int index) const override;

 private:
  bool empty_ = true;
};

#if defined(UI_QT)
class StyleMenuModel : public aui::SimpleMenuModel {
 public:
  StyleMenuModel();

  // views::MenuModel
  virtual void ActivatedAt(int index) override;
  virtual bool IsItemCheckedAt(int index) const override;
};

#endif  // defined(UI_QT)

class MainMenuModel final : private MainMenuContext,
                            private aui::SimpleMenuModel::Delegate,
                            public aui::SimpleMenuModel {
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
  aui::SimpleMenuModel table_submenu_;
  std::unique_ptr<FavouritesMenuModel> graph_favourites_;
  aui::SimpleMenuModel graph_submenu_;
  aui::SimpleMenuModel more_submenu_;
  PageMenuModel page_list_menu_;
  aui::SimpleMenuModel page_submenu_;
  WindowMenuModel window_list_menu_;
  TrashMenuModel trash_menu_;
  aui::SimpleMenuModel window_submenu_;
#if defined(UI_QT)
  StyleMenuModel style_submenu_;
#endif
  aui::SimpleMenuModel settings_submenu_;
  aui::SimpleMenuModel help_submenu_;
};
