#pragma once

#include "aui/models/tree_node_model.h"
#include "base/check.h"
#include "base/lifetime.h"
#include "common/node_state.h"
#include "portfolio/portfolio_manager.h"

class PortfolioManager;

class PortfolioTreeNode : public scada::aui::TreeNode<PortfolioTreeNode> {
 public:
  // The tree's invisible root, which stands for no portfolio at all. It used to
  // be built with the same constructor as the others, binding `portfolio_` to
  // `*static_cast<Portfolio*>(nullptr)`; forming that reference is undefined
  // behaviour whether or not anything reads it, and it made `is_portfolio()`
  // answer true for the root.
  explicit PortfolioTreeNode(PortfolioManager& portfolio_manager)
      : portfolio_manager_{portfolio_manager} {}

  PortfolioTreeNode(PortfolioManager& portfolio_manager,
                    const Portfolio& portfolio)
      : portfolio_manager_{portfolio_manager}, portfolio_{&portfolio} {}

  // Only valid on a node that stands for a portfolio -- see `is_portfolio()`.
  const Portfolio& portfolio() const SCADA_LIFETIME_BOUND {
    scada::base::Check(portfolio_, "portfolio() on the root node");
    return *portfolio_;
  }
  const scada::NodeId& item_id() const SCADA_LIFETIME_BOUND { return item_id_; }

  // True for the nodes that stand for a portfolio, as opposed to the item nodes
  // beneath them (which carry their parent's portfolio and a non-empty item id)
  // and the root (which carries no portfolio).
  bool is_portfolio() const {
    return portfolio_ && item_id_ == scada::NodeId();
  }

  void set_title(const std::u16string& title) { title_ = title; }
  void set_icon(int icon) { icon_ = icon; }
  void set_item_id(const scada::NodeId& item_id) { item_id_ = item_id; }

  // TreeNode.
  virtual std::u16string GetText(int column_id) const override {
    return title_;
  }
  virtual int GetIcon() const override { return icon_; }
  virtual void SetText(int column_id, const std::u16string& title) override;
  virtual bool IsEditable(int column_id) const override {
    return is_portfolio();
  }

 private:
  PortfolioManager& portfolio_manager_;
  const Portfolio* portfolio_ = nullptr;
  scada::NodeId item_id_;

  std::u16string title_;
  int icon_ = -1;
};

class PortfolioTreeModel : public scada::aui::TreeNodeModel<PortfolioTreeNode>,
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
