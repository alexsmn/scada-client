#pragma once

#include "node_service/node_observer.h"

#include <list>
#include <set>
#include <string>

class NodeService;
class Portfolio;

class PortfolioEvents {
 public:
  virtual void Portfolio_OnUpdate(Portfolio& portfolio) {}
  virtual void Portfolio_OnDelete(Portfolio& portfolio) {}
  virtual void Portfolio_OnUpdateItem(Portfolio& portfolio,
                                      const scada::NodeId& node_id) {}
  virtual void Portfolio_OnDeleteItem(Portfolio& portfolio,
                                      const scada::NodeId& node_id) {}
};

struct PortfolioManagerContext {
  NodeService& node_service_;
};

class PortfolioManager : private PortfolioManagerContext,
                         private NodeRefObserver {
 public:
  using Portfolios = std::list<Portfolio>;
  using PortfolioEventsSet = std::set<PortfolioEvents*>;

  explicit PortfolioManager(PortfolioManagerContext&& context);
  ~PortfolioManager();

  void Subscribe(PortfolioEvents& events);
  void Unsubscribe(PortfolioEvents& events);

  Portfolios::iterator Find(const Portfolio& portfolio);
  Portfolios::iterator Find(std::u16string_view name);

  Portfolio& New();
  void Rename(const Portfolio& portfolio, std::u16string_view name);
  void Delete(const Portfolio& portfolio);

  void AddItem(const Portfolio& portfolio, const scada::NodeId& item);
  void DeleteItem(const Portfolio& portfolio, const scada::NodeId& item);

  Portfolios portfolios;
  PortfolioEventsSet portfolio_events;

 private:
  void UpdateNode(const scada::NodeId& node_id);
  void DeleteNode(const scada::NodeId& node_id);

  // NodeRefObserver
  virtual void OnModelChanged(const scada::ModelChangeEvent& event) override;
  virtual void OnNodeSemanticChanged(const scada::NodeId& node_id) override;
};
