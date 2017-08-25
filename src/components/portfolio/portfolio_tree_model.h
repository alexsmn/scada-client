#pragma once

#include "core/configuration_types.h"
#include "client/services/portfolio_manager.h"
#include "ui/base/models/tree_node_model.h"
#include "common/node_ref.h"

class NodeRefService;

class PortfolioTreeNode : public ui::TreeNode<PortfolioTreeNode> {
 public:
  PortfolioTreeNode(PortfolioManager& portfolio_manager, const Portfolio& portfolio)
      : portfolio_manager_(portfolio_manager),
        portfolio_(portfolio) {}

  const Portfolio& portfolio() const { return portfolio_; }
  bool is_portfolio() const { return !node; }

  // TreeNode.
  virtual base::string16 GetText(int column_id) const { return title; }
  virtual int GetIcon() const { return icon; }
  virtual void SetTitle(const base::string16& title);

  NodeRef node;
  base::string16 title;
  int icon = -1;

 private:
  PortfolioManager& portfolio_manager_;
  const Portfolio& portfolio_;
};

class PortfolioTreeModel : public ui::TreeNodeModel<PortfolioTreeNode>,
                           protected PortfolioEvents {
 public:
  PortfolioTreeModel(PortfolioManager& portfolio_manager, NodeRefService& node_service);
  virtual ~PortfolioTreeModel();

  PortfolioTreeNode& root() { return *ui::TreeNodeModel<PortfolioTreeNode>::root();  }

  PortfolioTreeNode* FindPortfolioNode(const Portfolio& portfolio);
  PortfolioTreeNode* FindItemNode(PortfolioTreeNode& portfolio_node,
                                  const scada::NodeId& item_id);

 protected:
  // PortfolioEvents
  virtual void Portfolio_OnUpdate(Portfolio& portfolio) override;
  virtual void Portfolio_OnDelete(Portfolio& portfolio) override;
  virtual void Portfolio_OnUpdateItem(Portfolio& portfolio, const scada::NodeId& node_id) override;
  virtual void Portfolio_OnDeleteItem(Portfolio& portfolio, const scada::NodeId& node_id) override;

 private:
  void AddPortfolioNode(const Portfolio& portfolio);

  void AddItemNode(PortfolioTreeNode& portfolio_node, const scada::NodeId& item_id);

  NodeRefService& node_service_;
  PortfolioManager& portfolio_manager_;
};
