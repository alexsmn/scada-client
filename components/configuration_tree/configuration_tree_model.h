#pragma once

#include "components/configuration_tree/configuration_tree_node.h"
#include "components/configuration_tree/node_service_tree.h"
#include "node_service/node_observer.h"
#include "node_service/node_ref.h"
#include "ui/base/models/tree_node_model.h"

#include <map>
#include <memory>

class NodeService;

struct ConfigurationTreeModelContext {
  using ReferenceFilter = std::vector<
      std::pair<scada::NodeId /*reference_type_id*/, bool /*forward*/>>;

  NodeService& node_service_;
  std::unique_ptr<NodeServiceTree> node_service_tree_;
};

class ConfigurationTreeModel : private ConfigurationTreeModelContext,
                               public ui::TreeNodeModel<ConfigurationTreeNode>,
                               protected NodeRefObserver {
 public:
  explicit ConfigurationTreeModel(ConfigurationTreeModelContext&& context);
  virtual ~ConfigurationTreeModel();

  const NodeRef& root_node() const { return root()->node(); }

  void Init();

  typedef std::multimap<scada::NodeId, ConfigurationTreeNode*> TreeNodeMap;
  const TreeNodeMap& tree_node_map() const { return tree_node_map_; }

  ConfigurationTreeNode* FindTreeNode(const scada::NodeId& node_id,
                                      const scada::NodeId& reference_type_id,
                                      bool forward_reference);
  // TODO: Optimize.
  std::vector<ConfigurationTreeNode*> FindTreeNodes(
      const scada::NodeId& node_id);

 protected:
  std::unique_ptr<ConfigurationTreeNode> CreateTreeNodeIfMatches(
      const scada::NodeId& reference_type_id,
      bool forward_reference,
      const NodeRef& node);

  // Returns nullptr if node must be skipped.
  virtual std::unique_ptr<ConfigurationTreeNode> CreateTreeNode(
      const scada::NodeId& reference_type_id,
      bool forward_reference,
      const NodeRef& node);

  // NodeRefObserver
  virtual void OnModelChanged(const scada::ModelChangeEvent& event) override;
  virtual void OnNodeSemanticChanged(const scada::NodeId& node_id) override;

 private:
  void UpdateChildTreeNodes(const scada::NodeId& parent_id);
  void DeleteTreeNodes(const scada::NodeId& node_id);

  TreeNodeMap tree_node_map_;

  friend class ConfigurationTreeNode;
};
