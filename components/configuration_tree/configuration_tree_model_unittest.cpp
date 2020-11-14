#include "components/configuration_tree/configuration_tree_model.h"

#include "address_space/address_space.h"
#include "base/logger.h"
#include "core/standard_node_ids.h"
#include "node_service/address_space/address_space_node_service.h"

#include <gmock/gmock.h>

class TestAddressSpace : public scada::AddressSpace {
 public:
  virtual scada::Node* GetMutableNode(const scada::NodeId& node_id) override {
    return nullptr;
  }

  const scada::Node* GetNode(const scada::NodeId& node_id) const override {
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
  /*const auto logger = std::make_shared<NullLogger>();
  TestAddressSpace address_space;
  NodeFetchStatusChecker node_fetch_status_checker =
      [](const scada::NodeId& node_id) { return NodeFetchStatus{}; };
  NodeFetchHandler node_fetch_handler =
      [](const scada::NodeId& node_id,
         const NodeFetchStatus& requested_status) {};
  AddressSpaceNodeService node_service{
      {logger, node_fetch_status_checker, node_fetch_handler, address_space}};
  auto root_node = node_service.GetNode(scada::id::RootFolder);

  ConfigurationTreeModel model{
      node_service, task_manager, root_node, {scada::id::Organizes}, {}};

  ASSERT_NE(nullptr, model.GetRoot());
  CompareSubTree(root_node, model.GetRoot());*/
}
