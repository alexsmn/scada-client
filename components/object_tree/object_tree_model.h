#pragma once

#include "components/configuration_tree/configuration_tree_model.h"
#include "components/object_tree/visible_node_model.h"

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
};

class ObjectTreeModel : private ObjectTreeModelContext,
                        public ConfigurationTreeModel {
 public:
  explicit ObjectTreeModel(ObjectTreeModelContext&& context);

  void SetNodeVisible(void* tree_node, bool visible);

  // TreeModel
  virtual int GetColumnCount() const override;
  virtual std::wstring GetColumnText(int column_id) const override;
  virtual int GetColumnPreferredSize(int column_id) const override;
  virtual std::wstring GetText(void* tree_node, int column_id) override;
  virtual SkColor GetTextColor(void* tree_node, int column_id) override;
  virtual SkColor GetBackgroundColor(void* tree_node, int column_id) override;

 private:
  std::shared_ptr<VisibleNode> CreateVisibleNode(void* tree_node);
  std::shared_ptr<VisibleNode> CreateFetchedVisibleNode(const NodeRef& node);

  VisibleNodeModel visible_node_model_;
};
