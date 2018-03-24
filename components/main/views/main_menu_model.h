#pragma once

#include "ui/base/models/simple_menu_model.h"

class ActionManager;
class Favourites;
class MainWindowViews;
class WindowDefinition;

class OpenMenuModel : public ui::SimpleMenuModel {
 public:
  typedef std::vector<const WindowDefinition*> FavouritesVector;

  OpenMenuModel(ui::SimpleMenuModel::Delegate* delegate, const unsigned view_types[], Favourites& favourites);
 
  // views::MenuModel
  virtual void MenuWillShow() override;
  virtual void ActivatedAt(int index) override;
  
 private:
  void Rebuild();

  void FillFavourites();

  Favourites& favourites_;

  const unsigned* view_types_;

  int first_favourite_;
  FavouritesVector favourites_vector_;
};

class MainMenuModel : public ui::SimpleMenuModel,
                      private ui::SimpleMenuModel::Delegate {
 public:
  MainMenuModel(MainWindowViews& main_view, ActionManager& action_manager, Favourites& favourites);

 private:
  void Rebuild();

  // ui::SimpleMenuModel::Delegate
  virtual bool IsCommandIdChecked(int command_id) const override { return false; }
  virtual bool IsCommandIdEnabled(int command_id) const override { return false; }
  virtual bool GetAcceleratorForCommandId(
      int command_id,
      ui::Accelerator* accelerator) override { return false; }
  virtual void ExecuteCommand(int command_id) override;

  MainWindowViews& main_view_;
  ActionManager& action_manager_;
  Favourites& favourites_;
 
  std::unique_ptr<ui::MenuModel> context_menu_;
  std::unique_ptr<ui::SimpleMenuModel> data_submenu_;
  std::unique_ptr<OpenMenuModel> table_submenu_;
  std::unique_ptr<OpenMenuModel> graph_submenu_;
  ui::SimpleMenuModel more_submenu_;
};
