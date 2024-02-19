#pragma once

#include <memory>

class NodeService;
class PortfolioManager;
class Profile;

struct PortfolioModuleContext {
  NodeService& node_service_;
  Profile& profile_;
};

class PortfolioModule : private PortfolioModuleContext {
 public:
  explicit PortfolioModule(PortfolioModuleContext&& context);
  ~PortfolioModule();

  PortfolioManager& portfolio_manager() { return *portfolio_manager_; }

 private:
  std::unique_ptr<PortfolioManager> portfolio_manager_;
};