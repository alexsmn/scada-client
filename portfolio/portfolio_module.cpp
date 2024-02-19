#include "portfolio/portfolio_module.h"

#include "base/value_util.h"
#include "model/node_id_util.h"
#include "portfolio/portfolio.h"
#include "portfolio/portfolio_manager.h"
#include "profile/profile.h"

PortfolioModule::PortfolioModule(PortfolioModuleContext&& context)
    : PortfolioModuleContext{std::move(context)} {
  portfolio_manager_ = std::make_unique<PortfolioManager>(
      PortfolioManagerContext{node_service_});

  // portfolios
  if (auto* pfoliose = GetList(profile_.data(), "portfolios")) {
    auto& portfolios = portfolio_manager_->portfolios;
    for (auto& pfolioe : *pfoliose) {
      Portfolio& portfolio = portfolios.emplace_back();
      portfolio.name = GetString16(pfolioe, "name");
      // items
      if (auto* itemse = GetList(pfolioe, "items")) {
        for (auto& iteme : *itemse) {
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
    for (auto& portfolio : portfolio_manager_->portfolios) {
      base::Value pfolioe{base::Value::Type::DICTIONARY};
      SetKey(pfolioe, "name", portfolio.name);
      {
        base::Value::ListStorage item_storage;
        for (const auto& node_id : portfolio.items)
          item_storage.emplace_back(NodeIdToScadaString(node_id));
        pfolioe.SetKey("items", base::Value{std::move(item_storage)});
      }
      portfolio_storage.emplace_back(std::move(pfolioe));
    }
    data.SetKey("portfolios", base::Value{std::move(portfolio_storage)});
  });
}

PortfolioModule::~PortfolioModule() = default;
