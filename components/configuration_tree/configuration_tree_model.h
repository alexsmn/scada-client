#pragma once

#include "node_service/node_observer.h"
#include "node_service/node_ref.h"
#include "ui/base/dragdrop/drag_drop_types.h"
#include "ui/base/models/tree_node_model.h"

#include <map>
#include <memory>

class ConfigurationTreeModel;
class Executor;
class NodeService;
class TaskManager;

using DropAction = std::function<int()>;

class ConfigurationTreeNode : public ui::TreeNode<ConfigurationTreeNode> {
 public:
  ConfigurationTreeNode(ConfigurationTreeModel& model,
                        scada::NodeId reference_type_id,
                        bool forward_reference,
                        NodeRef node);
  virtual ~ConfigurationTreeNode();

  ConfigurationTreeModel& model() const { return model_; }
  const NodeRef& node() const { return node_; }
  const scada::NodeId& reference_type_id() const { return reference_type_id_; }
  bool forward_reference() const { return forward_reference_; }

  bool loaded() const { return loaded_; }
  void Load();

  // TreeNode
  virtual int GetChildCount() const override;
  virtual std::wstring GetText(int column_id) const override;
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
  const scada::NodeId reference_type_id_;
  const bool forward_reference_;
  const NodeRef node_;

  bool loaded_ = false;

  friend class ConfigurationTreeModel;
};

class ConfigurationTreeRootNode : public ConfigurationTreeNode {
 public:
  ConfigurationTreeRootNode(ConfigurationTreeModel& model, NodeRef tree);

  // TreeNode
  virtual std::wstring GetText(int column_id) const;
  virtual int GetIcon() const;
};

struct ConfigurationTreeModelContext {
  using ReferenceFilter = std::vector<
      std::pair<scada::NodeId /*reference_type_id*/, bool /*forward*/>>;

  NodeService& node_service_;
  TaskManager& task_manager_;
  const NodeRef root_node_;
  const ReferenceFilter reference_filter_;
  const std::vector<scada::NodeId> type_definition_ids_;
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

  int GetDropAction(const scada::NodeId& dragging_id,
                    ConfigurationTreeNode*& target_node,
                    DropAction& action);

  ConfigurationTreeNode* FindTreeNode(const scada::NodeId& node_id,
                                      const scada::NodeId& reference_type_id,
                                      bool forward_reference);
  // TODO: Optimize.
  std::vector<ConfigurationTreeNode*> FindTreeNodes(
      const scada::NodeId& node_id);

 protected:
  friend class ConfigurationTreeNode;

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
  void DeleteMissingTreeNodes(const scada::NodeId& node_id);
  void DeleteTreeNodes(const scada::NodeId& node_id);

  TreeNodeMap tree_node_map_;
};
