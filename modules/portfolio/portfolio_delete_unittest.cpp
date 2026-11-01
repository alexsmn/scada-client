#include "portfolio/portfolio_delete.h"

#include "aui/dialog_service_mock.h"
#include "base/test/awaitable_test.h"
#include "base/test/test_executor.h"
#include "model/node_id_util.h"
#include "node_service/static/static_node_service.h"
#include "portfolio/portfolio.h"
#include "portfolio/portfolio_manager.h"

#include <gmock/gmock.h>

using namespace testing;

namespace {

Awaitable<MessageBoxResult> Answer(MessageBoxResult result) {
  co_return result;
}

// Backlog 961: Delete used to remove a portfolio, or an object from one, at
// once, although neither can be brought back.
class PortfolioDeleteTest : public Test {
 protected:
  PortfolioDeleteTest() {
    Portfolio& portfolio = manager_.New();
    manager_.Rename(portfolio, u"Feeders");
    manager_.AddItem(portfolio, kFeeder);
  }

  void Run(std::optional<scada::NodeId> item_id, std::u16string item_title) {
    CoSpawn(executor_, [this, item_id = std::move(item_id),
                        item_title = std::move(item_title)]() mutable {
      return ConfirmAndDeleteFromPortfolio(dialog_service_, manager_,
                                           u"Feeders", std::move(item_id),
                                           std::move(item_title));
    });
    Drain(executor_);
  }

  void OperatorAnswers(MessageBoxResult result) {
    EXPECT_CALL(dialog_service_,
                RunMessageBox(_, _, MessageBoxMode::QuestionYesNo))
        .WillOnce(Return(ByMove(Answer(result))));
  }

  const scada::NodeId kFeeder = NodeIdFromScadaString("NS2.Feeder.1");

  TestExecutor executor_;
  StaticNodeService node_service_;
  PortfolioManager manager_{
      PortfolioManagerContext{.node_service_ = node_service_}};
  NiceMock<MockDialogService> dialog_service_;
};

TEST_F(PortfolioDeleteTest, DecliningKeepsThePortfolio) {
  OperatorAnswers(MessageBoxResult::No);
  Run(std::nullopt, {});
  ASSERT_EQ(manager_.portfolios.size(), 1u);
  EXPECT_THAT(manager_.portfolios.front().items, ElementsAre(kFeeder));
}

TEST_F(PortfolioDeleteTest, ConfirmingDeletesThePortfolio) {
  OperatorAnswers(MessageBoxResult::Yes);
  Run(std::nullopt, {});
  EXPECT_TRUE(manager_.portfolios.empty());
}

TEST_F(PortfolioDeleteTest, DecliningKeepsTheObject) {
  OperatorAnswers(MessageBoxResult::No);
  Run(kFeeder, u"Feeder 1");
  EXPECT_THAT(manager_.portfolios.front().items, ElementsAre(kFeeder));
}

TEST_F(PortfolioDeleteTest, ConfirmingRemovesTheObjectAndNamesIt) {
  std::u16string question;
  EXPECT_CALL(dialog_service_,
              RunMessageBox(_, _, MessageBoxMode::QuestionYesNo))
      .WillOnce([&question](std::u16string_view message, std::u16string_view,
                            MessageBoxMode) {
        question = message;
        return Answer(MessageBoxResult::Yes);
      });
  Run(kFeeder, u"Feeder 1");
  ASSERT_EQ(manager_.portfolios.size(), 1u);
  EXPECT_TRUE(manager_.portfolios.front().items.empty());
  EXPECT_NE(question.find(u"Feeder 1"), std::u16string::npos);
  EXPECT_NE(question.find(u"Feeders"), std::u16string::npos);
}

}  // namespace
