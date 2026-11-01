#pragma once

#include "base/awaitable.h"
#include "scada/node_id.h"

#include <optional>
#include <string>

class DialogService;
class PortfolioManager;

// Asks the operator to confirm, then deletes the portfolio named
// `portfolio_name` -- or, with `item_id`, removes that object from it, where
// `item_title` names the object in the question. Nothing is deleted unless the
// answer is Yes. Neither a portfolio nor its list can be restored afterwards,
// which is why the question is asked at all (backlog 961).
//
// The portfolio is looked up again by name once the question is answered, so
// one deleted meanwhile is simply not found. `dialog_service` and
// `portfolio_manager` must outlive the question; every parameter that is not a
// reference to one of them is taken by value, since the frame is lazy.
[[nodiscard]] Awaitable<void> ConfirmAndDeleteFromPortfolio(
    DialogService& dialog_service,
    PortfolioManager& portfolio_manager,
    std::u16string portfolio_name,
    std::optional<scada::NodeId> item_id,
    std::u16string item_title);
