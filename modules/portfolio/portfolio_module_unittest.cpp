#include "portfolio/portfolio_module.h"

#include "model/node_id_util.h"
#include "node_service/static/static_node_service.h"
#include "portfolio/portfolio.h"
#include "portfolio/portfolio_manager.h"

#include <boost/json.hpp>
#include <gtest/gtest.h>

namespace {

class PortfolioModuleTest : public testing::Test {
 protected:
  StaticNodeService node_service_;
  PortfolioManager manager_{
      PortfolioManagerContext{.node_service_ = node_service_}};
};

// Regression: the serializer wrote each item as a bare string while the loader
// read `{"path": ...}` objects, so names round-tripped and contents did not —
// every portfolio was empty after a restart, since the module first landed.
TEST_F(PortfolioModuleTest, ItemsSurviveASaveAndLoad) {
  const scada::NodeId feeder = NodeIdFromScadaString("NS2.Feeder.1");
  const scada::NodeId breaker = NodeIdFromScadaString("NS2.42");
  ASSERT_FALSE(feeder.is_null());
  ASSERT_FALSE(breaker.is_null());

  Portfolio& portfolio = manager_.New();
  manager_.Rename(portfolio, u"Feeders");
  portfolio.items = {feeder, breaker};

  boost::json::value profile_data{boost::json::object{}};
  SavePortfolios(manager_, profile_data);

  StaticNodeService node_service;
  PortfolioManager loaded{
      PortfolioManagerContext{.node_service_ = node_service}};
  LoadPortfolios(profile_data, loaded);

  ASSERT_EQ(1u, loaded.portfolios.size());
  EXPECT_EQ(u"Feeders", loaded.portfolios.front().name);
  EXPECT_EQ((Portfolio::Items{feeder, breaker}),
            loaded.portfolios.front().items);
}

// The shape the serializer used to write is still read, so a profile saved by
// an earlier build gets its portfolio contents back rather than an empty list.
TEST_F(PortfolioModuleTest, LoadsTheLegacyBareStringItems) {
  const boost::json::value profile_data = boost::json::parse(R"({
    "portfolios": [
      {"name": "Legacy", "items": ["NS2.Feeder.1", "not a node id", 7]}
    ]
  })");

  LoadPortfolios(profile_data, manager_);

  ASSERT_EQ(1u, manager_.portfolios.size());
  EXPECT_EQ(u"Legacy", manager_.portfolios.front().name);
  EXPECT_EQ((Portfolio::Items{NodeIdFromScadaString("NS2.Feeder.1")}),
            manager_.portfolios.front().items);
}

// A profile with no portfolios, or one that is not an object at all, loads
// nothing and never throws — it is a user-editable file.
TEST_F(PortfolioModuleTest, ToleratesAMissingOrMalformedList) {
  LoadPortfolios(boost::json::value{boost::json::object{}}, manager_);
  LoadPortfolios(boost::json::value{"junk"}, manager_);
  LoadPortfolios(boost::json::parse(R"({"portfolios": "junk"})"), manager_);
  LoadPortfolios(boost::json::parse(R"({"portfolios": [3, "x", {}]})"),
                 manager_);

  // The three entries of the last call are each a portfolio with no name and
  // no items; nothing before them produced one.
  EXPECT_EQ(3u, manager_.portfolios.size());
  for (const Portfolio& portfolio : manager_.portfolios) {
    EXPECT_TRUE(portfolio.name.empty());
    EXPECT_TRUE(portfolio.items.empty());
  }
}

}  // namespace
