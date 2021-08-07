#pragma once

#include "command_handler_impl.h"
#include "components/portfolio/portfolio.h"
#include "contents_model.h"
#include "controller.h"
#include "controller_context.h"
#include "selection_model.h"

#include <memory>

class PortfolioTreeModel;
class Tree;

class PortfolioView : protected ControllerContext,
                      public Controller,
                      public ContentsModel {
 public:
  explicit PortfolioView(const ControllerContext& context);
  virtual ~PortfolioView();

  // Controller
  virtual UiView* Init(const WindowDefinition& definition) override;
  virtual CommandHandler* GetCommandHandler(unsigned command_id) override;
  virtual SelectionModel* GetSelectionModel() override { return &selection_; }
  virtual ContentsModel* GetContentsModel() override { return this; }

  // ContentsModel
  virtual void AddContainedItem(const scada::NodeId& node_id,
                                unsigned flags) override;

 private:
  const Portfolio* GetSelectedPortfolio() const;

  std::wstring GetSelectionTitle();
  NodeIdSet GetSelectedNodeIdList();

  void DeleteSelection();
  void NewPortfolio();
  void AddItemsToPortfolio();

  SelectionModel selection_{{timed_data_service_}};

  std::unique_ptr<PortfolioTreeModel> model_;
  std::unique_ptr<Tree> tree_;

  CommandHandlerImpl command_handler_;
};
