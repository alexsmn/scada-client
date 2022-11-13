#pragma once

#include "base/boost_log.h"
#include "components/configuration_tree/configuration_tree_node.h"
#include "components/configuration_tree/node_service_tree.h"
#include "node_service/node_observer.h"
#include "node_service/node_ref.h"
#include "ui/base/models/tree_node_model.h"

#include <map>
#include <memory>

struct ConfigurationTreeModelContext {
  std::unique_ptr<NodeServiceTree> node_service_tree_;
};

class ConfigurationTreeModel : private ConfigurationTreeModelContext,
                               public aui::TreeNodeModel<ConfigurationTreeNode>,
                               private NodeRefObserver,
                               private NodeServiceTree::Observer {
 public:
  explicit ConfigurationTreeModel(ConfigurationTreeModelContext&& context);
  virtual ~ConfigurationTreeModel();

  const NodeRef& root_node() const { return root()->node(); }

  ConfigurationTreeNode* FindTreeNode(const scada::NodeId& node_id,
                                      const scada::NodeId& reference_type_id,
                                      bool forward_reference);

  ConfigurationTreeNode* FindFirstTreeNode(const scada::NodeId& node_id);

  // TODO: Optimize.
  std::vector<ConfigurationTreeNode*> FindTreeNodes(
      const scada::NodeId& node_id);

 protected:
  BoostLogger logger_{LOG_NAME("ConfigurationTreeModel")};

  // Returns nullptr if node must be skipped.
  virtual std::unique_ptr<ConfigurationTreeNode> CreateTreeNode(
      const scada::NodeId& reference_type_id,
      bool forward_reference,
      const NodeRef& node);

 private:
  std::unique_ptr<ConfigurationTreeNode> CreateTreeNodeIfMatches(
      const scada::NodeId& reference_type_id,
      bool forward_reference,
      const NodeRef& node);

  void UpdateChildTreeNodes(const scada::NodeId& parent_id);
  void UpdateChildTreeNodes(ConfigurationTreeNode& parent_tree_node);

  void DeleteTreeNodes(const scada::NodeId& node_id);

  // NodeServiceTree::Observer
  virtual void OnNodeDeleted(const scada::NodeId& node_id) override;
  virtual void OnNodeChildrenChanged(const scada::NodeId& node_id) override;
  virtual void OnNodeModelChanged(const scada::NodeId& node_id) override;
  virtual void OnNodeSemanticsChanged(const scada::NodeId& node_id) override;

  std::multimap<scada::NodeId, ConfigurationTreeNode*> tree_node_map_;

  friend class ConfigurationTreeNode;
};
