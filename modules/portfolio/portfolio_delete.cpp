#include "portfolio/portfolio_delete.h"

#include "aui/dialog_service.h"
#include "aui/translation.h"
#include "base/u16format.h"
#include "portfolio/portfolio.h"
#include "portfolio/portfolio_manager.h"

Awaitable<void> ConfirmAndDeleteFromPortfolio(
    DialogService& dialog_service,
    PortfolioManager& portfolio_manager,
    std::u16string portfolio_name,
    std::optional<scada::NodeId> item_id,
    std::u16string item_title) {
  const std::u16string message =
      item_id ? u16format(Translate("Remove \"{}\" from the portfolio \"{}\"?"),
                          item_title, portfolio_name)
              : u16format(Translate("Delete the portfolio \"{}\" with its "
                                    "list of objects? This cannot be undone."),
                          portfolio_name);
  const std::u16string title = item_id ? Translate("Remove from Portfolio")
                                       : Translate("Delete Portfolio");

  const MessageBoxResult answer = co_await dialog_service.RunMessageBox(
      message, title, MessageBoxMode::QuestionYesNo);
  if (answer != MessageBoxResult::Yes)
    co_return;

  auto portfolio = portfolio_manager.Find(portfolio_name);
  if (portfolio == portfolio_manager.portfolios.end())
    co_return;
  if (item_id)
    portfolio_manager.DeleteItem(*portfolio, *item_id);
  else
    portfolio_manager.Delete(*portfolio);
  co_return;
}
