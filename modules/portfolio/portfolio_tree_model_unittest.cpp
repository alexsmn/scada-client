#include "portfolio/portfolio_tree_model.h"

#include "node_service/static/static_node_service.h"
#include "portfolio/portfolio.h"
#include "portfolio/portfolio_manager.h"

#include <gtest/gtest.h>

namespace {

class PortfolioTreeModelTest : public testing::Test {
 protected:
  StaticNodeService node_service_;
  PortfolioManager portfolio_manager_{
      PortfolioManagerContext{.node_service_ = node_service_}};
  PortfolioTreeModel model_{node_service_, portfolio_manager_};
};

// The root stands for no portfolio. It was built by binding the node's
// `const Portfolio&` to `*static_cast<Portfolio*>(nullptr)`, which made this
// answer true — so `IsEditable` reported the root renameable and `SetText`
// would have passed the null reference to `PortfolioManager::Rename`.
TEST_F(PortfolioTreeModelTest, RootIsNotAPortfolio) {
  ASSERT_TRUE(model_.root());
  EXPECT_FALSE(model_.root()->is_portfolio());
  EXPECT_FALSE(model_.root()->IsEditable(0));
}

// The nodes the root does carry are portfolios, and each answers with the one
// it was built from.
TEST_F(PortfolioTreeModelTest, PortfolioNodesCarryTheirPortfolio) {
  Portfolio& portfolio = portfolio_manager_.New();
  portfolio_manager_.Rename(portfolio, u"Feeders");

  ASSERT_EQ(1, model_.root()->GetChildCount());
  PortfolioTreeNode& node = model_.root()->GetChild(0);
  EXPECT_TRUE(node.is_portfolio());
  EXPECT_EQ(&portfolio, &node.portfolio());
  EXPECT_EQ(u"Feeders", node.GetText(0));
}

// An item node hangs off a portfolio node, carries that portfolio, and is not
// itself a portfolio — the distinction `is_portfolio()` exists to draw.
TEST_F(PortfolioTreeModelTest, ItemNodesAreNotPortfolios) {
  Portfolio& portfolio = portfolio_manager_.New();
  const scada::NodeId item_id{1, 42};
  portfolio_manager_.AddItem(portfolio, item_id);

  PortfolioTreeNode* portfolio_node = model_.FindPortfolioNode(portfolio);
  ASSERT_TRUE(portfolio_node);
  ASSERT_EQ(1, portfolio_node->GetChildCount());

  PortfolioTreeNode& item_node = portfolio_node->GetChild(0);
  EXPECT_FALSE(item_node.is_portfolio());
  EXPECT_EQ(item_id, item_node.item_id());
  EXPECT_EQ(&portfolio, &item_node.portfolio());
}

}  // namespace
