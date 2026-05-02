#pragma once

#include "base/boost_log.h"
#include "configuration/tree/configuration_tree_node.h"
#include "configuration/tree/node_service_tree.h"
#include "aui/models/tree_node_model.h"
#include "node_service/node_observer.h"
#include "node_service/node_ref.h"

#include <memory>

class Executor;

struct ConfigurationTreeModelContext {
  std::shared_ptr<Executor> executor_;
  std::unique_ptr<NodeServiceTree> node_service_tree_;
};

class ConfigurationTreeModel : private ConfigurationTreeModelContext,
                               public aui::TreeNodeModel<ConfigurationTreeNode>,
                               private NodeRefObserver,
                               private NodeServiceTree::Observer {
 public:
  explicit ConfigurationTreeModel(ConfigurationTreeModelContext&& context);
  virtual ~ConfigurationTreeModel();

  // Init must be separate from constructor because it calls virtual methods.
  void Init();

  const NodeRef& root_node() const { return root()->node(); }

  ConfigurationTreeNode* FindTreeNode(const scada::NodeId& node_id,
                                      const scada::NodeId& reference_type_id,
                                      bool forward_reference);

  ConfigurationTreeNode* FindFirstTreeNode(const scada::NodeId& node_id);

  // TODO: Optimize.
  std::vector<ConfigurationTreeNode*> FindTreeNodes(
      const scada::NodeId& node_id);

  std::weak_ptr<void> GetLifetimeToken() const { return lifetime_token_; }

 protected:
  // Returns nullptr if node must be skipped.
  virtual std::unique_ptr<ConfigurationTreeNode> CreateTreeNode(
      const scada::NodeId& reference_type_id,
      bool forward_reference,
      const NodeRef& node);

  BoostLogger logger_{LOG_NAME("ConfigurationTreeModel")};

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
  std::shared_ptr<void> lifetime_token_ = std::make_shared<int>(0);

  friend class ConfigurationTreeNode;
};
