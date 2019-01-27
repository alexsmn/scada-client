#pragma once

#include <memory>

#include "command_handler.h"
#include "contents_model.h"
#include "controller.h"
#include "services/portfolio.h"

class PortfolioTreeModel;
class Tree;

class PortfolioView : public Controller,
                      public CommandHandler,
                      public ContentsModel {
 public:
  explicit PortfolioView(const ControllerContext& context);
  virtual ~PortfolioView();

  // Controller
  virtual UiView* Init(const WindowDefinition& definition) override;
  virtual bool IsCommandEnabled(unsigned command_id) const override;
  virtual CommandHandler* GetCommandHandler(unsigned command_id) override;
  virtual void ExecuteCommand(unsigned command_id) override;
  virtual ContentsModel* GetContentsModel() override { return this; }

  // ContentsModel
  virtual void AddContainedItem(const scada::NodeId& node_id,
                                unsigned flags) override;

 private:
  const Portfolio* GetSelectedPortfolio() const;

  base::string16 GetSelectionTitle();
  NodeIdSet GetSelectedNodeIdList();

  void DeleteSelection();
  void NewPortfolio();
  void AddItemsToPortfolio();

  std::unique_ptr<PortfolioTreeModel> model_;
  std::unique_ptr<Tree> tree_;
};
