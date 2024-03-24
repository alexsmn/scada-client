#include "portfolio/portfolio_module.h"

#include "base/value_util.h"
#include "controller/controller_registry.h"
#include "model/node_id_util.h"
#include "portfolio/portfolio.h"
#include "portfolio/portfolio_manager.h"
#include "portfolio/portfolio_view.h"
#include "profile/profile.h"

namespace {

constexpr WindowInfo kPortfolioWindowInfo = {.command_id = ID_PORTFOLIO_VIEW,
                                             .name = "Portfolio",
                                             .title = u"Портфолио",
                                             .flags = WIN_SING | WIN_INS,
                                             .size = {200, 400}};

}

PortfolioModule::PortfolioModule(PortfolioModuleContext&& context)
    : PortfolioModuleContext{std::move(context)} {
  portfolio_manager_ = std::make_unique<PortfolioManager>(
      PortfolioManagerContext{node_service_});

  controller_registry_.AddControllerFactory(
      kPortfolioWindowInfo, [&portfolio_manager = *portfolio_manager_](
                                const ControllerContext& context) {
        return std::make_unique<PortfolioView>(context, portfolio_manager);
      });

  // portfolios
  if (const base::Value::ListStorage* pfoliose =
          GetList(profile_.data(), "portfolios")) {
    for (const base::Value& pfolioe : *pfoliose) {
      Portfolio& portfolio = portfolio_manager_->portfolios.emplace_back();
      portfolio.name = GetString16(pfolioe, "name");
      // items
      if (const base::Value::ListStorage* itemse = GetList(pfolioe, "items")) {
        for (const base::Value& iteme : *itemse) {
          auto path = GetString(iteme, "path");
          if (auto node_id = NodeIdFromScadaString(path); !node_id.is_null()) {
            portfolio.items.insert(node_id);
          }
        }
      }
    }
  }

  profile_.RegisterSerializer([this](base::Value& data) {
    base::Value::ListStorage portfolio_storage;
    for (const Portfolio& portfolio : portfolio_manager_->portfolios) {
      base::Value pfolioe{base::Value::Type::DICTIONARY};
      SetKey(pfolioe, "name", portfolio.name);
      {
        base::Value::ListStorage item_storage;
        for (const scada::NodeId& node_id : portfolio.items) {
          item_storage.emplace_back(NodeIdToScadaString(node_id));
        }
        pfolioe.SetKey("items", base::Value{std::move(item_storage)});
      }
      portfolio_storage.emplace_back(std::move(pfolioe));
    }
    data.SetKey("portfolios", base::Value{std::move(portfolio_storage)});
  });
}

PortfolioModule::~PortfolioModule() = default;
