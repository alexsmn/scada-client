#pragma once

#include "components/configuration_tree/configuration_tree_model.h"
#include "components/object_tree/visible_node_model.h"

struct ObjectTreeModelContext {
  NodeService& node_service_;
  TaskManager& task_manager_;
  const NodeRef root_;
  TimedDataService& timed_data_service_;
  Profile& profile_;
};

class ObjectTreeModel : private ObjectTreeModelContext,
                        public ConfigurationTreeModel {
 public:
  explicit ObjectTreeModel(ObjectTreeModelContext&& context);

  void SetNodeVisible(ConfigurationTreeNode& tree_node, bool visible);

  // TreeModel
  virtual int GetColumnCount() const override;
  virtual std::wstring GetColumnText(int column_id) const override;
  virtual int GetColumnPreferredSize(int column_id) const override;
  virtual std::wstring GetText(void* node, int column_id) override;
  virtual SkColor GetTextColor(void* node, int column_id) override;
  virtual SkColor GetBackgroundColor(void* node, int column_id) override;

 private:
  VisibleNodeModel visible_node_model_;
};
