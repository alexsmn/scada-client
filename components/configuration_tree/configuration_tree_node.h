#pragma once

#include "node_service/node_ref.h"
#include "ui/base/models/tree_node_model.h"

#include <map>
#include <memory>

namespace scada {
struct ModelChangeEvent;
}

class ConfigurationTreeModel;

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

  void Changed();

  virtual void OnModelChanged() {}

 private:
  void LoadChildren();

  ConfigurationTreeModel& model_;
  const scada::NodeId reference_type_id_;
  const bool forward_reference_;
  const NodeRef node_;

  bool children_loaded_ = false;

  friend class ConfigurationTreeModel;
};

class ConfigurationTreeRootNode : public ConfigurationTreeNode {
 public:
  ConfigurationTreeRootNode(ConfigurationTreeModel& model, NodeRef tree);

  // TreeNode
  virtual std::wstring GetText(int column_id) const override;
  virtual int GetIcon() const override;
};
