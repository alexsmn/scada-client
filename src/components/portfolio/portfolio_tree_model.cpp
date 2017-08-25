#include "components/portfolio/portfolio_tree_model.h"

#include "base/strings/sys_string_conversions.h"
#include "services/portfolio.h"
#include "common/node_ref_service.h"

void PortfolioTreeNode::SetTitle(const base::string16& title) {
  if (!is_portfolio())
    return;

  portfolio_manager_.Rename(portfolio_, title.c_str());
}

PortfolioTreeModel::PortfolioTreeModel(PortfolioManager& portfolio_manager, NodeRefService& node_service)
    : portfolio_manager_(portfolio_manager),
      node_service_(node_service) {
  set_root(std::make_unique<PortfolioTreeNode>(portfolio_manager_, *static_cast<Portfolio*>(NULL)));

  for (auto& portfolio : portfolio_manager_.portfolios)
    AddPortfolioNode(portfolio);

  portfolio_manager_.Subscribe(*this);
}

PortfolioTreeModel::~PortfolioTreeModel() {
  portfolio_manager_.Unsubscribe(*this);  
}

void PortfolioTreeModel::AddPortfolioNode(const Portfolio& portfolio) {
  auto node = std::make_unique<PortfolioTreeNode>(portfolio_manager_, portfolio);
  auto* node_ptr = node.get();
  node->title = portfolio.name;
  node->icon = 0;
  Add(root(), root().GetChildCount(), std::move(node));

  for (auto node_id : portfolio.items)
    AddItemNode(*node_ptr, node_id);
}

PortfolioTreeNode* PortfolioTreeModel::FindPortfolioNode(
                                       const Portfolio& portfolio) {
  for (int i = 0; i < root().GetChildCount(); ++i) {
    PortfolioTreeNode& node = root().GetChild(i);
    if (&node.portfolio() == &portfolio)
      return &node;
  }
  return NULL;
}

void PortfolioTreeModel::AddItemNode(PortfolioTreeNode& portfolio_node, const scada::NodeId& item_id) {
  auto node_ref = node_service_.GetPartialNode(item_id);
  auto node = std::make_unique<PortfolioTreeNode>(portfolio_manager_, portfolio_node.portfolio());
  node->title = base::SysNativeMBToWide(node_ref.browse_name());
  node->icon = 1;
  node->node = node_ref;
  Add(portfolio_node, portfolio_node.GetChildCount(), std::move(node));
}

PortfolioTreeNode* PortfolioTreeModel::FindItemNode(
                                       PortfolioTreeNode& portfolio_node,
                                       const scada::NodeId& item_id) {
  for (int i = 0; i < portfolio_node.GetChildCount(); ++i) {
    PortfolioTreeNode& node = portfolio_node.GetChild(i);
    if (node.node.id() == item_id)
      return &node;
  }
  return NULL;
}

void PortfolioTreeModel::Portfolio_OnUpdate(Portfolio& portfolio) {
  PortfolioTreeNode* node = FindPortfolioNode(portfolio);
  if (!node)
    AddPortfolioNode(portfolio);
  else {
    node->title = portfolio.name;
    TreeNodeChanged(node);
  }
}

void PortfolioTreeModel::Portfolio_OnDelete(Portfolio& portfolio) {
  PortfolioTreeNode* node = FindPortfolioNode(portfolio);
  assert(node);
  Remove(root(), root().IndexOfChild(*node));
  delete node;
}

void PortfolioTreeModel::Portfolio_OnUpdateItem(Portfolio& portfolio, const scada::NodeId& node_id) {
  PortfolioTreeNode* portfolio_node = FindPortfolioNode(portfolio);
  assert(portfolio_node);

  PortfolioTreeNode* node = FindItemNode(*portfolio_node, node_id);
  if (!node)
    AddItemNode(*portfolio_node, node_id);
  else {
    node->title = base::SysNativeMBToWide(node->node.browse_name());
    TreeNodeChanged(node);
  }
}

void PortfolioTreeModel::Portfolio_OnDeleteItem(Portfolio& portfolio, const scada::NodeId& node_id) {
  PortfolioTreeNode* portfolio_node = FindPortfolioNode(portfolio);
  assert(portfolio_node);

  PortfolioTreeNode* node = FindItemNode(*portfolio_node, node_id);
  assert(node);

  Remove(*portfolio_node, portfolio_node->IndexOfChild(*node));
  delete node;
}
