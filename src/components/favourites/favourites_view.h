#pragma once

#include <memory>

#include "controller.h"

class FavouritesTreeModel;
class Tree;
class WindowDefinition;

class FavouritesView : public Controller {
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
