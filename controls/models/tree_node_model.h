#pragma once

#include "controls/models/tree_model.h"

#include <algorithm>
#include <cassert>
#include <memory>
#include <vector>

namespace aui {

template <class NodeType>
class TreeNode {
 public:
  TreeNode() {}
  virtual ~TreeNode() {}

  TreeNode(const TreeNode&) = delete;
  TreeNode& operator=(const TreeNode&) = delete;

  NodeType* parent() const { return parent_; }

  virtual int GetChildCount() const {
    return static_cast<int>(children_.size());
  }

  NodeType& GetChild(int index) { return *children_[index]; }
  const NodeType& GetChild(int index) const { return *children_[index]; }

  virtual std::u16string GetText(int column_id) const = 0;
  virtual int GetIcon() const { return -1; }

  virtual void SetText(int column_id, const std::u16string& title) {}
  virtual bool IsEditable(int column_id) const { return false; }
  virtual bool IsSelectable(int column_id) const { return true; }
  virtual EditData GetEditData(int column_id) const { return {}; };
  virtual void HandleEditButton(int column_id) const {}

  virtual Color GetTextColor(int column_id) const { return ColorCode::Black; }

  virtual Color GetBackgroundColor(int column_id) const {
    return ColorCode::Transparent;
  }

  virtual bool HasChildren() const { return true; }

  virtual bool CanFetchMore() const { return false; }

  virtual void FetchMore() {}

  void Add(int index, std::unique_ptr<NodeType> child) {
    assert(!child->parent_);
    child->parent_ = reinterpret_cast<NodeType*>(this);
    children_.emplace(children_.begin() + index, std::move(child));
  }

  std::unique_ptr<NodeType> Remove(int index) {
    auto node = std::move(children_[index]);
    assert(node->parent_ == this);
    node->parent_ = nullptr;
    children_.erase(children_.begin() + index);
    return node;
  }

  void Remove(int index, int count) {
    children_.erase(children_.begin() + index,
                    children_.begin() + index + count);
  }

  int IndexOfChild(const NodeType& child) const {
    auto i = std::find_if(children_.begin(), children_.end(),
                          [&](auto& ptr) { return ptr.get() == &child; });
    return i != children_.end() ? static_cast<int>(i - children_.begin()) : -1;
  }

 private:
  NodeType* parent_ = nullptr;
  std::vector<std::unique_ptr<NodeType>> children_;
};

// TreeNodeWithValue ----------------------------------------------------------

template <class ValueType>
class TreeNodeWithValue : public TreeNode<TreeNodeWithValue<ValueType>> {
 public:
  TreeNodeWithValue() {}

  TreeNodeWithValue(const TreeNodeWithValue&) = delete;
  TreeNodeWithValue& operator=(const TreeNodeWithValue&) = delete;

  explicit TreeNodeWithValue(const ValueType& value)
      : ParentType({}), value(value) {}

  TreeNodeWithValue(const std::u16string& title, const ValueType& value)
      : ParentType(title), value(value) {}

  ValueType value;

 private:
  typedef TreeNode<TreeNodeWithValue<ValueType>> ParentType;
};

template <class NodeType>
class TreeNodeModel : public TreeModel {
 public:
  TreeNodeModel() {}
  explicit TreeNodeModel(std::unique_ptr<NodeType> root)
      : root_(std::move(root)) {}

  void set_root(std::unique_ptr<NodeType> root) { root_ = std::move(root); }

  NodeType* root() { return root_.get(); }
  const NodeType* root() const { return root_.get(); }

  NodeType* AsNode(void* node) { return reinterpret_cast<NodeType*>(node); }
  const NodeType* AsNode(void* node) const {
    return reinterpret_cast<const NodeType*>(node);
  }

  void Add(NodeType& parent, int index, std::unique_ptr<NodeType> child) {
    TreeNodesAdding(&parent, index, 1);
    parent.Add(index, std::move(child));
    TreeNodesAdded(&parent, index, 1);
  }

  std::unique_ptr<NodeType> Remove(NodeType& parent, int index) {
    TreeNodesDeleting(&parent, index, 1);
    auto node = parent.Remove(index);
    TreeNodesDeleted(&parent, index, 1);
    return node;
  }

  void Remove(NodeType& parent, int index, int count) {
    TreeNodesDeleting(&parent, index, count);
    parent.Remove(index, count);
    TreeNodesDeleted(&parent, index, count);
  }

  // TreeModel
  virtual void* GetRoot() override { return root_.get(); }
  virtual void* GetParent(void* node) override {
    return AsNode(node)->parent();
  }
  virtual int GetChildCount(void* parent) override {
    return AsNode(parent)->GetChildCount();
  }
  virtual void* GetChild(void* parent, int index) override {
    return &AsNode(parent)->GetChild(index);
  }
  virtual std::u16string GetText(void* node, int column_id) override {
    return AsNode(node)->GetText(column_id);
  }
  virtual int GetIcon(void* node) override { return AsNode(node)->GetIcon(); }
  virtual void SetText(void* node,
                       int column_id,
                       const std::u16string& text) override {
    AsNode(node)->SetText(column_id, text);
  }
  virtual bool IsEditable(void* node, int column_id) const override {
    return AsNode(node)->IsEditable(column_id);
  }
  virtual bool IsSelectable(void* node, int column_id) const override {
    return AsNode(node)->IsSelectable(column_id);
  }
  virtual EditData GetEditData(void* node, int column_id) const override {
    return AsNode(node)->GetEditData(column_id);
  };
  virtual void HandleEditButton(void* node, int column_id) const override {
    return AsNode(node)->HandleEditButton(column_id);
  }
  virtual Color GetTextColor(void* node, int column_id) override {
    return AsNode(node)->GetTextColor(column_id);
  }
  virtual Color GetBackgroundColor(void* node, int column_id) override {
    return AsNode(node)->GetBackgroundColor(column_id);
  }
  virtual bool HasChildren(void* parent) const override {
    return AsNode(parent)->HasChildren();
  }
  virtual bool CanFetchMore(void* parent) const override {
    return AsNode(parent)->CanFetchMore();
  }
  virtual void FetchMore(void* parent) override { AsNode(parent)->FetchMore(); }

 private:
  std::unique_ptr<NodeType> root_;
};

}  // namespace aui
