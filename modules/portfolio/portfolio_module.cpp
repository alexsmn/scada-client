#include "portfolio/portfolio_module.h"

#include "aui/translation.h"
#include "base/value_util.h"
#include "controller/action.h"
#include "controller/command_ui_registry.h"
#include "controller/controller_registry.h"
#include "model/node_id_util.h"
#include "portfolio/portfolio.h"
#include "portfolio/portfolio_manager.h"
#include "portfolio/portfolio_view.h"
#include "profile/profile.h"
#include "resources/common_resources.h"

namespace {

constexpr WindowInfo kPortfolioWindowInfo = {.command_id = ID_PORTFOLIO_VIEW,
                                             .name = "Portfolio",
                                             .title = u"Portfolio",
                                             .flags = WIN_SING | WIN_INS,
                                             .size = {200, 400}};

}  // namespace

void RegisterPortfolioCommandActions(UiCommandRegistry& ui_command_registry) {
  ui_command_registry.AddAction(
      Action{.command_id_ = ID_NEW_PORTFOLIO,
             .category_ = CATEGORY_EDIT,
             .title_ = Translate("Create Portfolio")});
  ui_command_registry.AddAction(Action{.command_id_ = ID_ADD_ITEMS,
                                       .category_ = CATEGORY_EDIT,
                                       .title_ = Translate("Add Items..."),
                                       .short_title_ = Translate("Add Items")});
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

  ui_command_registry_.AddMenuItem({.menu_id = MainMenuId::More,
                                    .order = 140,
                                    .command_id = ID_PORTFOLIO_VIEW,
                                    .title = Translate("Portfolio"),
                                    .checkable = true});
  RegisterPortfolioCommandActions(ui_command_registry_);

  LoadPortfolios(profile_.data(), *portfolio_manager_);

  profile_.RegisterSerializer([this](boost::json::value& data) {
    SavePortfolios(*portfolio_manager_, data);
  });
}

PortfolioModule::~PortfolioModule() = default;

void LoadPortfolios(const boost::json::value& profile_data,
                    PortfolioManager& portfolio_manager) {
  const auto* pfoliose = GetList(profile_data, "portfolios");
  if (!pfoliose) {
    return;
  }

  for (const auto& pfolioe : *pfoliose) {
    Portfolio& portfolio = portfolio_manager.portfolios.emplace_back();
    portfolio.name = GetString16(pfolioe, "name");
    const auto* itemse = GetList(pfolioe, "items");
    if (!itemse) {
      continue;
    }
    for (const auto& iteme : *itemse) {
      // An item is `{"path": "<id>"}`. A bare `"<id>"` string is what the
      // serializer wrote until 2026-09-07 — a shape this loader never read, so
      // every portfolio came back empty after a restart — and is still
      // accepted so those profiles recover their contents.
      std::string_view path = iteme.is_string()
                                  ? std::string_view{iteme.as_string()}
                                  : GetString(iteme, "path");
      if (auto node_id = NodeIdFromScadaString(path); !node_id.is_null()) {
        portfolio.items.insert(node_id);
      }
    }
  }
}

void SavePortfolios(const PortfolioManager& portfolio_manager,
                    boost::json::value& profile_data) {
  boost::json::array portfolio_storage;
  for (const Portfolio& portfolio : portfolio_manager.portfolios) {
    boost::json::value pfolioe{boost::json::object{}};
    SetKey(pfolioe, "name", portfolio.name);
    boost::json::array item_storage;
    for (const scada::NodeId& node_id : portfolio.items) {
      boost::json::value iteme{boost::json::object{}};
      SetKey(iteme, "path", NodeIdToScadaString(node_id));
      item_storage.emplace_back(std::move(iteme));
    }
    pfolioe.as_object()["items"] = std::move(item_storage);
    portfolio_storage.emplace_back(std::move(pfolioe));
  }
  profile_data.as_object()["portfolios"] = std::move(portfolio_storage);
}
