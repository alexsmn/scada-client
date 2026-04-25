#pragma once

#include "configuration/configuration_module.h"
#include "configuration/tree/configuration_tree_view.h"
#include "controller/contents_observer.h"
#include "aui/models/tree_model.h"

#include <optional>
#include <vector>

class ConfigurationTreeNode;
class ObjectTreeModel;

class ObjectTreeView : public ConfigurationTreeView,
                       protected aui::TreeModelObserver,
                       private ContentsObserver {
 public:
  ObjectTreeView(const ControllerContext& context,
                 const NodeServiceTreeFactory& node_service_tree_factory);
  virtual ~ObjectTreeView();

  std::optional<std::u16string> GetFirstValueTextForTesting();
  std::vector<std::u16string> GetExpandedLabelPathForTesting(int levels);

 protected:
  void UpdateNodesVisibility(ConfigurationTreeNode& parent_node, bool expanded);

  // TreeModelObserver
  virtual void OnTreeNodeChanged(void* node) override;
  virtual void OnTreeNodesAdded(void* parent, int start, int count) override;
  virtual void OnTreeNodesDeleting(void* parent, int start, int count) override;
  virtual void OnTreeModelResetting() override;

 private:
  ObjectTreeModel& model();

  // ContentsObserver
  virtual void OnContentsChanged(const NodeIdSet& node_ids) override;
  virtual void OnContainedItemChanged(const scada::NodeId& item_id,
                                      bool added) override;

  static std::shared_ptr<ConfigurationTreeModel> CreateConfigurationTreeModel(
      const ControllerContext& context,
      const NodeServiceTreeFactory& node_service_tree_factory);

  static std::unique_ptr<ConfigurationTreeDropHandler> CreateTreeDropHandler(
      const ControllerContext& context);

  ConfigurationTreeNode* value_node_for_testing_ = nullptr;
  int value_node_change_count_for_testing_ = 0;
};
