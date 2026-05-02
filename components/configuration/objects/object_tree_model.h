#pragma once

#include "configuration/configuration_module.h"
#include "configuration/tree/configuration_tree_model.h"
#include "configuration/objects/visible_node_model.h"

#include <memory>

class Executor;
class NodeService;

struct ObjectTreeModelContext {
  const std::shared_ptr<Executor> executor_;
  NodeService& node_service_;
  const NodeRef root_;
  TimedDataService& timed_data_service_;
  Profile& profile_;
  BlinkerManager& blinker_manager_;
  const NodeServiceTreeFactory& node_service_tree_factory_;
};

class ObjectTreeModel : private ObjectTreeModelContext,
                        public ConfigurationTreeModel {
 public:
  explicit ObjectTreeModel(ObjectTreeModelContext&& context);

  void SetNodeVisible(void* tree_node, bool visible);

  // TreeModel
  virtual int GetColumnCount() const override;
  virtual std::u16string GetColumnText(int column_id) const override;
  virtual int GetColumnPreferredSize(int column_id) const override;
  virtual std::u16string GetText(void* tree_node, int column_id) override;
  virtual aui::Color GetTextColor(void* tree_node, int column_id) override;
  virtual aui::Color GetBackgroundColor(void* tree_node,
                                        int column_id) override;

 private:
  virtual std::unique_ptr<ConfigurationTreeNode> CreateTreeNode(
      const scada::NodeId& reference_type_id,
      bool forward_reference,
      const NodeRef& node) override;
  virtual std::shared_ptr<VisibleNode> CreateFetchedVisibleNode(
      const NodeRef& node);

 private:
  std::shared_ptr<VisibleNode> CreateVisibleNode(void* tree_node);
  static Awaitable<void> CompleteVisibleNodeFetchAsync(
      std::shared_ptr<Executor> executor,
      std::weak_ptr<void> lifetime_token,
      ObjectTreeModel& model,
      std::shared_ptr<ProxyVisibleNode> proxy_visible_node,
      void* tree_node,
      NodeRef node,
      scada::NodeId node_id,
      scada::NodeId reference_type_id,
      bool forward_reference);

  VisibleNodeModel visible_node_model_;

  class ObjectTreeNode;
};
