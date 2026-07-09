#pragma once

#include "base/lifetime.h"

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
