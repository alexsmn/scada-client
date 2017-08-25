#pragma once

#include "base/strings/string16.h"
#include "core/configuration_types.h"
#include "core/node_observer.h"
#include "common/node_ref_observer.h"

#include <list>
#include <set>

class NodeRefService;
class Portfolio;

class PortfolioEvents {
 public:
  virtual void Portfolio_OnUpdate(Portfolio& portfolio) {}
  virtual void Portfolio_OnDelete(Portfolio& portfolio) {}
  virtual void Portfolio_OnUpdateItem(Portfolio& portfolio, const scada::NodeId& node_id) {}
  virtual void Portfolio_OnDeleteItem(Portfolio& portfolio, const scada::NodeId& node_id) {}
};

class PortfolioManager : private NodeRefObserver {
 public:
  typedef std::list<Portfolio> Portfolios;
  typedef std::set<PortfolioEvents*> PortfolioEventsSet;

  explicit PortfolioManager(NodeRefService& node_service);
  ~PortfolioManager();

  void Subscribe(PortfolioEvents& events);
  void Unsubscribe(PortfolioEvents& events);

  Portfolios::iterator Find(const Portfolio& portfolio);
  Portfolios::iterator Find(const base::char16* name);

  Portfolio& New();
  void Rename(const Portfolio& portfolio, const base::char16* name);
  void Delete(const Portfolio& portfolio);

  void AddItem(const Portfolio& portfolio, const scada::NodeId& item);
  void DeleteItem(const Portfolio& portfolio, const scada::NodeId& item);

  Portfolios portfolios;
  PortfolioEventsSet portfolio_events;

 private:
  void UpdateNode(const scada::NodeId& node_id);
  void DeleteNode(const scada::NodeId& node_id);

  // scada::NodeObserver
  virtual void OnNodeAdded(const scada::NodeId& node_id) override;
  virtual void OnNodeDeleted(const scada::NodeId& node_id) override;
  virtual void OnNodeSemanticChanged(const scada::NodeId& node_id) override;

  NodeRefService& node_service_;
};
