#pragma once

#include "scada/node_id.h"

#include <boost/signals2/connection.hpp>
#include <list>
#include <set>
#include <string>

namespace scada {
struct ModelChangeEvent;
}

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

class PortfolioManager : private PortfolioManagerContext {
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

  void OnModelChanged(const scada::ModelChangeEvent& event);
  void OnNodeSemanticChanged(const scada::NodeId& node_id);

  boost::signals2::scoped_connection model_changed_connection_;
  boost::signals2::scoped_connection node_semantic_changed_connection_;
};
