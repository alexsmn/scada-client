#pragma once

#include "base/lifetime.h"

#include <boost/json/fwd.hpp>
#include <memory>

class ControllerRegistry;
class NodeService;
class PortfolioManager;
class Profile;
class UiCommandRegistry;

struct PortfolioModuleContext {
  NodeService& node_service_;
  Profile& profile_;
  ControllerRegistry& controller_registry_;
  UiCommandRegistry& ui_command_registry_;
};

class PortfolioModule : private PortfolioModuleContext {
 public:
  explicit PortfolioModule(PortfolioModuleContext&& context);
  ~PortfolioModule();

  PortfolioManager& portfolio_manager() SCADA_LIFETIME_BOUND {
    return *portfolio_manager_;
  }

 private:
  std::unique_ptr<PortfolioManager> portfolio_manager_;
};

void RegisterPortfolioCommandActions(UiCommandRegistry& ui_command_registry);

// The profile's `portfolios` list: each entry is `{"name": ..., "items":
// [{"path": "<node id>"}, ...]}`. Split out of the module so the round trip
// is testable without a `Profile` or the registries.
//
// Appends every portfolio found under `profile_data` to `portfolio_manager`.
// Unknown or malformed entries are skipped, never fatal — the profile is a
// file the user can edit.
void LoadPortfolios(const boost::json::value& profile_data,
                    PortfolioManager& portfolio_manager);

// Writes `portfolio_manager`'s portfolios under `profile_data["portfolios"]`,
// replacing whatever was there. `profile_data` must be a JSON object.
void SavePortfolios(const PortfolioManager& portfolio_manager,
                    boost::json::value& profile_data);
