#pragma once

#include "aui/models/tree_model.h"
#include "configuration/configuration_module.h"
#include "configuration/tree/configuration_tree_view.h"
#include "controller/node_id_set.h"

#include <boost/signals2/connection.hpp>
#include <optional>
#include <vector>

class ConfigurationTreeNode;
class ObjectTreeModel;

class ObjectTreeView : public ConfigurationTreeView {
 public:
  ObjectTreeView(const ControllerContext& context,
                 const NodeServiceTreeFactory& node_service_tree_factory);
  virtual ~ObjectTreeView();

  std::optional<std::u16string> GetFirstValueTextForTesting();
  std::vector<std::u16string> GetExpandedLabelPathForTesting(int levels);

 protected:
  void UpdateNodesVisibility(ConfigurationTreeNode& parent_node, bool expanded);

  void OnTreeNodeChanged(void* node);
  void OnTreeNodesAdded(void* parent, int start, int count);
  void OnTreeNodesDeleting(void* parent, int start, int count);
  void OnTreeModelResetting();

 private:
  ObjectTreeModel& model();

  void OnContentsChanged(const NodeIdSet& node_ids);
  void OnContainedItemChanged(const scada::NodeId& item_id, bool added);

  static std::shared_ptr<ConfigurationTreeModel> CreateConfigurationTreeModel(
      const ControllerContext& context,
      const NodeServiceTreeFactory& node_service_tree_factory);

  static std::unique_ptr<ConfigurationTreeDropHandler> CreateTreeDropHandler(
      const ControllerContext& context);

  ConfigurationTreeNode* value_node_for_testing_ = nullptr;
  int value_node_change_count_for_testing_ = 0;

  std::vector<boost::signals2::scoped_connection> model_connections_;
};
