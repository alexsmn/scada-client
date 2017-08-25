#include "client/services/portfolio_manager.h"

#include "base/logging.h"
#include "base/strings/stringprintf.h"
#include "common/scada_node_ids.h"
#include "client/services/portfolio.h"
#include "common/node_ref_service.h"

PortfolioManager::PortfolioManager(NodeRefService& node_service)
    : node_service_{node_service} {
  node_service_.AddObserver(*this);
}

PortfolioManager::~PortfolioManager() {
  node_service_.RemoveObserver(*this);
}

void PortfolioManager::OnNodeAdded(const scada::NodeId& node_id) {
  UpdateNode(node_id);
}

void PortfolioManager::OnNodeDeleted(const scada::NodeId& node_id) {
  DeleteNode(node_id);
}

void PortfolioManager::OnNodeSemanticChanged(const scada::NodeId& node_id) {
  UpdateNode(node_id);
}

void PortfolioManager::UpdateNode(const scada::NodeId& node_id) {
  for (auto& portfolio : portfolios) {
    if (portfolio.items.erase(node_id)) {
      for (auto* events : portfolio_events)
        events->Portfolio_OnUpdateItem(portfolio, node_id); 
    }
  }
}

void PortfolioManager::DeleteNode(const scada::NodeId& node_id) {
  for (Portfolios::iterator i = portfolios.begin(); i != portfolios.end(); ++i) {
    Portfolio& portfolio = *i;
    if (portfolio.items.erase(node_id)) {
      std::string item_path = node_id.ToString();
      LOG(INFO) << "Portfolio " << portfolio.name << ": "
                << "Item " << item_path << " is removed";
          
      for (PortfolioEventsSet::iterator ei = portfolio_events.begin();
                                        ei != portfolio_events.end(); ++ei)
        (*ei)->Portfolio_OnDeleteItem(portfolio, node_id);
    }
  }
}

void PortfolioManager::Subscribe(PortfolioEvents& events) {
  portfolio_events.insert(&events);
}

void PortfolioManager::Unsubscribe(PortfolioEvents& events) {
  portfolio_events.erase(&events);
}

PortfolioManager::Portfolios::iterator PortfolioManager::Find(
                                       const Portfolio& portfolio) {
  for (Portfolios::iterator i = portfolios.begin(); i != portfolios.end(); ++i)
    if (&*i == &portfolio)
      return i;				
  return portfolios.end();
}

PortfolioManager::Portfolios::iterator PortfolioManager::Find(const base::char16* name) {
  for (Portfolios::iterator i = portfolios.begin(); i != portfolios.end(); ++i)
    if (i->name.compare(name) == 0)
      return i;				
  return portfolios.end();
}

Portfolio& PortfolioManager::New() {
  static const base::char16 mask[] = L"Портфолио";
  
  base::string16 name = mask;
  int id = 2;
  while (Find(name.c_str()) != portfolios.end())
    name = base::StringPrintf(L"%ls %d", mask, id++);

  portfolios.push_back(Portfolio());
  Portfolio& portfolio = portfolios.back();
  portfolio.name = name;
  
  for (auto* events : portfolio_events)
    events->Portfolio_OnUpdate(portfolio);
    
  return portfolio;
}

void PortfolioManager::Rename(const Portfolio& portfolio, const base::char16* name) {
  assert(Find(portfolio) != portfolios.end());

  Portfolio& p = const_cast<Portfolio&>(portfolio);
  p.name = name;

  for (auto* events : portfolio_events)
    events->Portfolio_OnUpdate(p);
}

void PortfolioManager::Delete(const Portfolio& portfolio) {
  Portfolios::iterator p = Find(portfolio);
  assert(p != portfolios.end());
  for (auto* events : portfolio_events)
    events->Portfolio_OnDelete(const_cast<Portfolio&>(portfolio));
  portfolios.erase(p);
}

void PortfolioManager::AddItem(const Portfolio& portfolio, const scada::NodeId& item) {
  Portfolio& p = const_cast<Portfolio&>(portfolio);
  p.items.insert(item);
 
  for (auto* events : portfolio_events)
    events->Portfolio_OnUpdateItem(p, item);
}

void PortfolioManager::DeleteItem(const Portfolio& portfolio, const scada::NodeId& item) {
  Portfolio& p = const_cast<Portfolio&>(portfolio);
  p.items.erase(item);

  for (auto* events : portfolio_events)
    events->Portfolio_OnDeleteItem(p, item);
}
