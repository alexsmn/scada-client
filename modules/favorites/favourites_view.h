#pragma once

#include "base/awaitable.h"
#include "resources/common_resources.h"
#include "controller/action_manager.h"
#include "controller/controller.h"
#include "controller/controller_context.h"

#include <memory>

namespace aui {
class Tree;
}

class Favourites;
class FavouritesTreeModel;
class WindowDefinition;

class FavouritesView final : protected ControllerContext, public Controller {
 public:
  FavouritesView(const ControllerContext& context, Favourites& favorites);
  ~FavouritesView();

  // Controller events
  virtual std::unique_ptr<UiView> Init(
      const WindowDefinition& definition) override;
  virtual void Save(WindowDefinition& definition) override;
  virtual ActionManager* GetActionManager() override;

 private:
  void OpenSelection();
  void DeleteSelection();

#if !defined(UI_WT)
  void AddUrl();
#endif

  Favourites& favourites_;
  std::shared_ptr<void> lifetime_token_ = std::make_shared<int>(0);

  const std::shared_ptr<FavouritesTreeModel> favourites_tree_model_;

  aui::Tree* tree_view_ = nullptr;

  ActionManager command_registry_;
  Action& open_command_ = command_registry_.AddAction(ID_OPEN);
  Action& rename_command_ = command_registry_.AddAction(ID_RENAME);
  Action& delete_command_ = command_registry_.AddAction(ID_DELETE);

#if !defined(UI_WT)
  Action& add_url_command_ =
      command_registry_.AddAction(ID_FAVOURITES_ADD_URL);
#endif
};
