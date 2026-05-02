#pragma once

#include "aui/models/tree_node_model.h"
#include "base/awaitable.h"
#include "node_service/node_ref.h"

#include <map>
#include <memory>

namespace scada {
class NodeId;
struct ModelChangeEvent;
}

class ConfigurationTreeModel;
class Executor;

class ConfigurationTreeNode : public aui::TreeNode<ConfigurationTreeNode> {
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
  virtual std::u16string GetText(int column_id) const override;
  virtual int GetIcon() const override;
  virtual bool HasChildren() const override;
  virtual bool CanFetchMore() const override;
  virtual void FetchMore() override;

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

  // Attaches one level of children already present in the address space
  // to this tree node. Intentionally not called from the ctor — see the
  // ctor comment for the stack-overflow reason — but the root node
  // invokes it explicitly so the tree shows its first level without
  // needing Qt to call FetchMore first.
  int AddChildren();

  virtual void OnModelChanged() {}

 private:
  static Awaitable<void> CompleteFetchMoreAsync(
      std::shared_ptr<Executor> executor,
      std::weak_ptr<void> lifetime_token,
      ConfigurationTreeModel& model,
      NodeRef node,
      scada::NodeId node_id,
      scada::NodeId reference_type_id,
      bool forward_reference);

  ConfigurationTreeModel& model_;
  const scada::NodeId reference_type_id_;
  const bool forward_reference_;
  const NodeRef node_;

  bool children_requested_ = false;
  bool children_loaded_ = false;

  friend class ConfigurationTreeModel;
};

class ConfigurationTreeRootNode : public ConfigurationTreeNode {
 public:
  ConfigurationTreeRootNode(ConfigurationTreeModel& model, NodeRef tree);

  // TreeNode
  virtual std::u16string GetText(int column_id) const override;
  virtual int GetIcon() const override;
};
