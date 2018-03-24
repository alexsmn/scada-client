#pragma once

#include "core/configuration_types.h"
#include "services/portfolio_manager.h"
#include "ui/base/models/tree_node_model.h"

class PortfolioManager;

class PortfolioTreeNode : public ui::TreeNode<PortfolioTreeNode> {
 public:
  PortfolioTreeNode(PortfolioManager& portfolio_manager,
                    const Portfolio& portfolio)
      : portfolio_manager_{portfolio_manager}, portfolio_{portfolio} {}

  const Portfolio& portfolio() const { return portfolio_; }
  const scada::NodeId& item_id() const { return item_id_; }
  bool is_portfolio() const { return item_id_ == scada::NodeId(); }

  void set_title(const base::string16& title) { title_ = title; }
  void set_icon(int icon) { icon_ = icon; }
  void set_item_id(const scada::NodeId& item_id) { item_id_ = item_id; }

  // TreeNode.
  virtual base::string16 GetText(int column_id) const { return title_; }
  virtual int GetIcon() const { return icon_; }
  virtual void SetTitle(const base::string16& title);

 private:
  PortfolioManager& portfolio_manager_;
  const Portfolio& portfolio_;
  scada::NodeId item_id_;

  base::string16 title_;
  int icon_ = -1;
};

class PortfolioTreeModel : public ui::TreeNodeModel<PortfolioTreeNode>,
                           protected PortfolioEvents {
 public:
  PortfolioTreeModel(NodeService& node_service,
                     PortfolioManager& portfolio_manager);
  virtual ~PortfolioTreeModel();

  PortfolioTreeNode* FindPortfolioNode(const Portfolio& portfolio);
  PortfolioTreeNode* FindItemNode(PortfolioTreeNode& portfolio_node,
                                  const scada::NodeId& item_id);

 protected:
  // PortfolioEvents
  virtual void Portfolio_OnUpdate(Portfolio& portfolio) override;
  virtual void Portfolio_OnDelete(Portfolio& portfolio) override;
  virtual void Portfolio_OnUpdateItem(Portfolio& portfolio,
                                      const scada::NodeId& node_id) override;
  virtual void Portfolio_OnDeleteItem(Portfolio& portfolio,
                                      const scada::NodeId& node_id) override;

 private:
  void AddPortfolioNode(const Portfolio& portfolio);

  void AddItemNode(PortfolioTreeNode& portfolio_node,
                   const scada::NodeId& item_id);

  NodeService& node_service_;
  PortfolioManager& portfolio_manager_;
};
