#pragma once

#include <map>
#include <memory>

#include "common/node_observer.h"
#include "common/node_ref.h"
#include "ui/base/dragdrop/drag_drop_types.h"
#include "ui/base/models/tree_node_model.h"

class ConfigurationTreeModel;
class NodeService;
class TaskManager;

using DropAction = std::function<int()>;

class ConfigurationTreeNode : public ui::TreeNode<ConfigurationTreeNode> {
 public:
  ConfigurationTreeNode(ConfigurationTreeModel& model,
                        const NodeRef& data_node);
  virtual ~ConfigurationTreeNode();

  ConfigurationTreeModel& model() const { return model_; }
  const NodeRef& data_node() const { return data_node_; }

  bool loaded() const { return loaded_; }
  void Load();

  // TreeNode
  virtual int GetChildCount() const override;
  virtual base::string16 GetText(int column_id) const override;
  virtual int GetIcon() const override;

 protected:
  enum {
    IMAGE_FOLDER,
    IMAGE_ITEM,
    IMAGE_DEVICE_RUNNING,
    IMAGE_DEVICE_STOPPED,
    IMAGE_SUBSYSTEM_RUNNING,
    IMAGE_SUBSYSTEM_STOPPED,
    IMAGE_DEVICE,
    IMAGE_DEVICE_DISABLED,
  };

  virtual void OnModelChanged(const scada::ModelChangeEvent& event) {}

  void Changed();

 private:
  ConfigurationTreeModel& model_;
  const NodeRef data_node_;

  bool loaded_ = false;

  friend class ConfigurationTreeModel;
};

class ConfigurationTreeRootNode : public ConfigurationTreeNode {
 public:
  ConfigurationTreeRootNode(ConfigurationTreeModel& model, NodeRef tree);

  // TreeNode
  virtual base::string16 GetText(int column_id) const;
  virtual int GetIcon() const;
};

class ConfigurationTreeModel : public ui::TreeNodeModel<ConfigurationTreeNode>,
                               protected NodeRefObserver {
 public:
  ConfigurationTreeModel(NodeService& node_service,
                         TaskManager& task_manager,
                         NodeRef tree,
                         std::vector<scada::NodeId> reference_type_ids,
                         std::vector<scada::NodeId> type_definition_ids);
  virtual ~ConfigurationTreeModel();

  NodeRef tree() const { return tree_; }

  void Init();

  ConfigurationTreeNode* FindNode(const scada::NodeId& node_id);

  typedef std::map<scada::NodeId, ConfigurationTreeNode*> NodeMap;
  const NodeMap& node_map() const { return node_map_; }

  int GetDropAction(const scada::NodeId& dragging_id,
                    ConfigurationTreeNode*& target_node,
                    DropAction& action);

 protected:
  friend class ConfigurationTreeNode;

  std::unique_ptr<ConfigurationTreeNode> CreateNodeIfMatches(
      const NodeRef& data_node);

  // Returns nullptr if node must be skipped.
  virtual std::unique_ptr<ConfigurationTreeNode> CreateNode(
      const NodeRef& data_node);

  // NodeRefObserver
  virtual void OnModelChanged(const scada::ModelChangeEvent& event) override;
  virtual void OnNodeSemanticChanged(const scada::NodeId& node_id) override;

 private:
  void UpdateNode(const scada::ModelChangeEvent& event);

  NodeService& node_service_;
  TaskManager& task_manager_;
  const NodeRef tree_;
  const std::vector<scada::NodeId> reference_type_ids_;
  const std::vector<scada::NodeId> type_definition_ids_;

  NodeMap node_map_;
};

inline ConfigurationTreeNode* ConfigurationTreeModel::FindNode(
    const scada::NodeId& node_id) {
  if (node_id.is_null())
    return nullptr;
  auto i = node_map_.find(node_id);
  return i != node_map_.end() ? i->second : nullptr;
}
