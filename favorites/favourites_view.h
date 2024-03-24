#pragma once

#include "base/promise.h"
#include "common_resources.h"
#include "controller/command_registry.h"
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
  virtual CommandHandler* GetCommandHandler(unsigned command_id) override;

 private:
  void OpenSelection();
  void DeleteSelection();

#if !defined(UI_WT)
  promise<> AddUrl();
#endif

  Favourites& favourites_;

  const std::shared_ptr<FavouritesTreeModel> favourites_tree_model_;

  aui::Tree* tree_view_ = nullptr;

  CommandRegistry command_registry_;
  Command& open_command_ = command_registry_.AddCommand(ID_OPEN);
  Command& rename_command_ = command_registry_.AddCommand(ID_RENAME);
  Command& delete_command_ = command_registry_.AddCommand(ID_DELETE);

#if !defined(UI_WT)
  Command& add_url_command_ =
      command_registry_.AddCommand(ID_FAVOURITES_ADD_URL);
#endif
};
