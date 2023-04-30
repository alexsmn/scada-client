#pragma once

#include "base/promise.h"
#include "command_registry.h"
#include "common_resources.h"
#include "controller.h"
#include "controller_context.h"

#include <memory>

namespace aui {
class Tree;
}

class FavouritesTreeModel;
class WindowDefinition;

class FavouritesView : protected ControllerContext, public Controller {
 public:
  explicit FavouritesView(const ControllerContext& context);
  ~FavouritesView();

  // Controller events
  virtual UiView* Init(const WindowDefinition& definition) override;
  virtual void Save(WindowDefinition& definition) override;
  virtual CommandHandler* GetCommandHandler(unsigned command_id) override;

 private:
  void OpenSelection();
  void DeleteSelection();

#if !defined(UI_WT)
  promise<> AddUrl();
#endif

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
