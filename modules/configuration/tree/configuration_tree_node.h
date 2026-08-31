#pragma once

#include "base/any_executor.h"
#include "base/lifetime.h"

#include "aui/models/tree_node_model.h"
#include "base/awaitable.h"
#include "node_service/node_ref.h"

#include <map>
#include <memory>

namespace scada {
class NodeId;
struct ModelChangeEvent;
}  // namespace scada

class ConfigurationTreeModel;

class ConfigurationTreeNode
    : public scada::aui::TreeNode<ConfigurationTreeNode> {
 public:
  ConfigurationTreeNode(ConfigurationTreeModel& model,
                        scada::NodeId reference_type_id,
                        bool forward_reference,
                        NodeRef node);
  virtual ~ConfigurationTreeNode();

  ConfigurationTreeModel& model() const { return model_; }
  const NodeRef& node() const SCADA_LIFETIME_BOUND { return node_; }
  const scada::NodeId& reference_type_id() const SCADA_LIFETIME_BOUND {
    return reference_type_id_;
  }
  bool forward_reference() const { return forward_reference_; }

  // True for the stand-in row a root draws while its own first level is still
  // in flight — see ConfigurationTreeLoadingNode. Such a row stands for no
  // node, so it is registered in neither the model's node map nor anything
  // that looks a row up by node id.
  virtual bool IsLoadingPlaceholder() const { return false; }

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
    // Number of tile indices, so the glyph table that resolves them
    // (`kItemGlyphs`) can be checked for coverage rather than silently
    // handing a row a null icon.
    IMAGE_COUNT,
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
      AnyExecutor executor,
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

// The row a root shows in place of the children it is still waiting for.
//
// Every ConfigurationTreeView hides its root row — the dock's title already
// names the root, see the ctor comment in configuration_tree_view.cpp — and the
// "[Loading]" suffix ConfigurationTreeNode::GetText appends is part of that
// row's text. So from the moment the root's first level is requested until it
// arrives there is nothing on screen at all, and a slow server is
// indistinguishable from an empty folder. This row is that missing feedback.
//
// It stands for no node: its NodeRef is null, which every NodeRef accessor
// answers safely, and it offers neither children nor a fetch of its own, so a
// walk that recurses through the tree stops at it rather than trying to expand
// a row with nothing behind it.
class ConfigurationTreeLoadingNode : public ConfigurationTreeNode {
 public:
  explicit ConfigurationTreeLoadingNode(ConfigurationTreeModel& model);

  virtual bool IsLoadingPlaceholder() const override;

  // TreeNode
  virtual std::u16string GetText(int column_id) const override;
  virtual int GetIcon() const override;
  virtual bool HasChildren() const override;
  virtual bool CanFetchMore() const override;
  virtual bool IsSelectable(int column_id) const override;
  virtual scada::aui::ColorRole GetColorRole(int column_id) const override;
};
