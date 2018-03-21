#include <gmock/gmock.h>

#include "base/logger.h"
#include "common/address_space_node_service.h"
#include "common/scada_node_ids.h"
#include "components/configuration_tree/configuration_tree_model.h"
#include "core/configuration.h"

class TestConfiguration : public scada::Configuration {
 public:
  scada::Node* GetNode(const scada::NodeId& node_id) override {
    return nullptr;
  }

  virtual void Subscribe(scada::NodeObserver& events) const override {}
  virtual void Unsubscribe(scada::NodeObserver& events) const override {}

  virtual void SubscribeNode(const scada::NodeId& node_id,
                             scada::NodeObserver& events) const override {}
  virtual void UnsubscribeNode(const scada::NodeId& node_id,
                               scada::NodeObserver& events) const override {}
};

void CompareSubTree(const NodeRef& node, void* tree_node) {}

TEST(ConfigurationTreeModel, CompareTree) {
  const auto logger = std::make_shared<NullLogger>();
  TestConfiguration configuration;
  NodeFetchStatusChecker node_fetch_status_checker =
      [](const scada::NodeId& node_id) { return NodeFetchStatus{}; };
  NodeFetchHandler node_fetch_handler =
      [](const scada::NodeId& node_id,
         const NodeFetchStatus& requested_status) {};
  AddressSpaceNodeService node_service{
      {logger, node_fetch_status_checker, node_fetch_handler, configuration}};
  auto root_node = node_service.GetNode(scada::id::RootFolder);

  /*ConfigurationTreeModel model{
      node_service, task_manager, root_node, {scada::id::Organizes}, {}};

  ASSERT_NE(nullptr, model.GetRoot());
  CompareSubTree(root_node, model.GetRoot());*/
}
