#pragma once

#include <memory>

#include "command_handler_impl.h"
#include "controller.h"

class Tree;
class FavouritesTreeModel;
class WindowDefinition;

class FavouritesView : public Controller {
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

  std::unique_ptr<FavouritesTreeModel> favourites_tree_model_;
  std::unique_ptr<Tree> tree_view_;

  CommandHandlerImpl command_handler_;
  Command& open_command_ = command_handler_.AddCommand(ID_OPEN);
  Command& rename_command_ = command_handler_.AddCommand(ID_RENAME);
  Command& delete_command_ = command_handler_.AddCommand(ID_DELETE);
};
