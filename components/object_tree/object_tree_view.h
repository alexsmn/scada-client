#pragma once

#include "components/configuration_tree/configuration_tree_view.h"
#include "contents_observer.h"
#include "aui/models/tree_model.h"

class ConfigurationTreeNode;
class ObjectTreeModel;

class ObjectTreeView : public ConfigurationTreeView,
                       protected aui::TreeModelObserver,
                       private ContentsObserver {
 public:
  explicit ObjectTreeView(const ControllerContext& context);
  virtual ~ObjectTreeView();

 protected:
  void UpdateNodesVisibility(ConfigurationTreeNode& parent_node, bool expanded);

  // TreeModelObserver
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
      const ControllerContext& context);

  static std::unique_ptr<ConfigurationTreeDropHandler> CreateTreeDropHandler(
      const ControllerContext& context);
};
