#include "portfolio/portfolio_tree_model.h"

#include "base/strings/sys_string_conversions.h"
#include "portfolio/portfolio.h"
#include "node_service/node_service.h"
#include "node_service/node_util.h"

void PortfolioTreeNode::SetText(int column_id, const std::u16string& title) {
  if (!is_portfolio())
    return;

  portfolio_manager_.Rename(portfolio_, title.c_str());
}

PortfolioTreeModel::PortfolioTreeModel(NodeService& node_service,
                                       PortfolioManager& portfolio_manager)
    : node_service_{node_service}, portfolio_manager_{portfolio_manager} {
  set_root(std::make_unique<PortfolioTreeNode>(
      portfolio_manager_, *static_cast<Portfolio*>(nullptr)));

  PortfolioManager::Portfolios& list = portfolio_manager_.portfolios;
  for (PortfolioManager::Portfolios::const_iterator i = list.begin();
       i != list.end(); ++i) {
    AddPortfolioNode(*i);
  }

  portfolio_manager_.Subscribe(*this);
}

PortfolioTreeModel::~PortfolioTreeModel() {
  portfolio_manager_.Unsubscribe(*this);
}

void PortfolioTreeModel::AddPortfolioNode(const Portfolio& portfolio) {
  auto node =
      std::make_unique<PortfolioTreeNode>(portfolio_manager_, portfolio);
  node->set_title(portfolio.name);
  node->set_icon(0);
  auto* node_ptr = node.get();
  Add(*root(), root()->GetChildCount(), std::move(node));

  for (std::set<scada::NodeId>::const_iterator i = portfolio.items.begin();
       i != portfolio.items.end(); ++i) {
    AddItemNode(*node_ptr, *i);
  }
}

PortfolioTreeNode* PortfolioTreeModel::FindPortfolioNode(
    const Portfolio& portfolio) {
  for (int i = 0; i < root()->GetChildCount(); ++i) {
    PortfolioTreeNode& node = root()->GetChild(i);
    if (&node.portfolio() == &portfolio)
      return &node;
  }
  return NULL;
}

void PortfolioTreeModel::AddItemNode(PortfolioTreeNode& portfolio_node,
                                     const scada::NodeId& item_id) {
  auto node = std::make_unique<PortfolioTreeNode>(portfolio_manager_,
                                                  portfolio_node.portfolio());
  node->set_title(GetDisplayName(node_service_, item_id));
  node->set_icon(1);
  node->set_item_id(item_id);
  Add(portfolio_node, portfolio_node.GetChildCount(), std::move(node));
}

PortfolioTreeNode* PortfolioTreeModel::FindItemNode(
    PortfolioTreeNode& portfolio_node,
    const scada::NodeId& item_id) {
  for (int i = 0; i < portfolio_node.GetChildCount(); ++i) {
    PortfolioTreeNode& node = portfolio_node.GetChild(i);
    if (node.item_id() == item_id)
      return &node;
  }
  return NULL;
}

void PortfolioTreeModel::Portfolio_OnUpdate(Portfolio& portfolio) {
  PortfolioTreeNode* node = FindPortfolioNode(portfolio);
  if (!node)
    AddPortfolioNode(portfolio);
  else {
    node->set_title(portfolio.name);
    TreeNodeChanged(node);
  }
}

void PortfolioTreeModel::Portfolio_OnDelete(Portfolio& portfolio) {
  PortfolioTreeNode* node = FindPortfolioNode(portfolio);
  assert(node);
  Remove(*root(), root()->IndexOfChild(*node));
}

void PortfolioTreeModel::Portfolio_OnUpdateItem(Portfolio& portfolio,
                                                const scada::NodeId& node_id) {
  PortfolioTreeNode* portfolio_node = FindPortfolioNode(portfolio);
  assert(portfolio_node);

  PortfolioTreeNode* node = FindItemNode(*portfolio_node, node_id);
  if (!node)
    AddItemNode(*portfolio_node, node_id);
  else {
    node->set_title(GetDisplayName(node_service_, node_id));
    TreeNodeChanged(node);
  }
}

void PortfolioTreeModel::Portfolio_OnDeleteItem(Portfolio& portfolio,
                                                const scada::NodeId& node_id) {
  PortfolioTreeNode* portfolio_node = FindPortfolioNode(portfolio);
  assert(portfolio_node);

  PortfolioTreeNode* node = FindItemNode(*portfolio_node, node_id);
  assert(node);

  Remove(*portfolio_node, portfolio_node->IndexOfChild(*node));
}
