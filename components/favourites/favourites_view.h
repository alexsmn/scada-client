#pragma once

#include <memory>

#include "command_handler.h"
#include "controller.h"

class Tree;
class FavouritesTreeModel;
class WindowDefinition;

class FavouritesView : public Controller, public CommandHandler {
 public:
  explicit FavouritesView(const ControllerContext& context);
  ~FavouritesView();

  // Controller events
  virtual UiView* Init(const WindowDefinition& definition) override;
  virtual void Save(WindowDefinition& definition) override;
  virtual CommandHandler* GetCommandHandler(unsigned command_id) override;
  virtual void ExecuteCommand(unsigned command) override;

 private:
  void OpenSelection();
  void DeleteSelection();

  std::unique_ptr<FavouritesTreeModel> favourites_tree_model_;
  std::unique_ptr<Tree> tree_view_;
};
