#pragma once

#include <map>
#include <memory>

#include "base/memory/weak_ptr.h"
#include "core/node_observer.h"
#include "common/node_ref.h"
#include "common/node_ref_observer.h"
#include "ui/base/models/tree_node_model.h"

class NodeRefService;
class ConfigurationTreeModel;

class ConfigurationTreeNode : public ui::TreeNode<ConfigurationTreeNode> {
 public:
  ConfigurationTreeNode(ConfigurationTreeModel& model, NodeRef data_node);
  virtual ~ConfigurationTreeNode();
  
  ConfigurationTreeModel& model() const { return model_; }

  const NodeRef& data_node() const { return data_node_; }

  void BrowseChildren();

  virtual void OnNodeSemanticChanged();
  
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

  void Changed();

 private:
  void SetDataNode(NodeRef data_node);

  ConfigurationTreeModel& model_;
  NodeRef data_node_;

  bool children_ = false;
};

class ConfigurationTreeModel : public ui::TreeNodeModel<ConfigurationTreeNode>,
                               protected NodeRefObserver {
 public:
  ConfigurationTreeModel(NodeRefService& node_service, const scada::NodeId& root_id,
      std::vector<scada::NodeId> reference_type_ids);
  virtual ~ConfigurationTreeModel();

  ConfigurationTreeNode* FindNode(const scada::NodeId& node_id);

  const NodeRef& root_node() const { return root()->data_node(); }

  typedef std::map<scada::NodeId, ConfigurationTreeNode*> NodeMap;
  const NodeMap& node_map() const { return node_map_; }

  virtual bool AreChecksVisible() const { return false; }

 protected:
  friend class ConfigurationTreeNode;

  // Returns nullptr if node must be skipped.
  virtual std::unique_ptr<ConfigurationTreeNode> CreateNode(const NodeRef& data_node);

  void EnsureParent(const scada::NodeId& node_id);
  void EnsureNode(const scada::NodeId& node_id);
  void EnsureNode2(const scada::NodeId& node_id, const scada::NodeId& parent_id);

  // NodeRefObserver
  virtual void OnNodeAdded(const scada::NodeId& node_id) override;
  virtual void OnNodeDeleted(const scada::NodeId& node_id) override;
  virtual void OnReferenceAdded(const scada::NodeId& node_id) override;
  virtual void OnReferenceDeleted(const scada::NodeId& node_id) override;
  virtual void OnNodeSemanticChanged(const scada::NodeId& node_id) override;

  NodeRefService& node_service_;

 private:
  void Browse(const NodeRef& node);
  void OnBrowseComplete(const scada::NodeId& node_id, const scada::ReferenceDescriptions& references);

  const std::vector<scada::NodeId> reference_type_ids_;

  NodeMap node_map_;

  base::WeakPtrFactory<ConfigurationTreeModel> weak_ptr_factory_{this};
};
