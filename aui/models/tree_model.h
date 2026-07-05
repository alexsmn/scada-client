#pragma once

#include "aui/color.h"
#include "aui/models/edit_data.h"

#include <boost/signals2/connection.hpp>
#include <boost/signals2/signal.hpp>
#include <functional>

namespace aui {

class TreeModel {
 public:
  using NodeRangeCallback =
      std::function<void(void* parent, int start, int count)>;
  using NodeChangedCallback = std::function<void(void* node)>;
  using ResetCallback = std::function<void()>;

  TreeModel() {}
  virtual ~TreeModel() {}

  TreeModel(const TreeModel&) = delete;
  TreeModel& operator=(const TreeModel&) = delete;

  [[nodiscard]] boost::signals2::scoped_connection SubscribeNodesAdding(
      const NodeRangeCallback& callback) {
    return nodes_adding_signal_.connect(callback);
  }
  [[nodiscard]] boost::signals2::scoped_connection SubscribeNodesAdded(
      const NodeRangeCallback& callback) {
    return nodes_added_signal_.connect(callback);
  }
  [[nodiscard]] boost::signals2::scoped_connection SubscribeNodesDeleting(
      const NodeRangeCallback& callback) {
    return nodes_deleting_signal_.connect(callback);
  }
  [[nodiscard]] boost::signals2::scoped_connection SubscribeNodesDeleted(
      const NodeRangeCallback& callback) {
    return nodes_deleted_signal_.connect(callback);
  }
  [[nodiscard]] boost::signals2::scoped_connection SubscribeNodeChanged(
      const NodeChangedCallback& callback) {
    return node_changed_signal_.connect(callback);
  }
  [[nodiscard]] boost::signals2::scoped_connection SubscribeModelResetting(
      const ResetCallback& callback) {
    return model_resetting_signal_.connect(callback);
  }
  [[nodiscard]] boost::signals2::scoped_connection SubscribeModelReset(
      const ResetCallback& callback) {
    return model_reset_signal_.connect(callback);
  }

  virtual void* GetRoot() = 0;
  virtual int GetColumnCount() const { return 1; }
  virtual std::u16string GetColumnText(int column_id) const {
    return std::u16string();
  }
  virtual int GetColumnPreferredSize(int column_id) const { return 0; }

  virtual void* GetParent(void* node) = 0;
  virtual int GetChildCount(void* parent) { return 0; }
  virtual void* GetChild(void* parent, int index) { return NULL; }
  virtual std::u16string GetText(void* node, int column_id) {
    return std::u16string();
  }
  virtual int GetIcon(void* node) { return -1; }
  virtual Color GetTextColor(void* node, int column_id) {
    return ColorCode::Transparent;
  }
  virtual Color GetBackgroundColor(void* node, int column_id) {
    return ColorCode::Transparent;
  }

  virtual void SetText(void* node, int column_id, const std::u16string& text) {}
  // TODO: Remove. `IsEditable` should be covered by `GetEditData` none editor
  // type.
  virtual bool IsEditable(void* node, int column_id) const { return false; }
  virtual bool IsSelectable(void* node, int column_id) const { return true; }
  virtual EditData GetEditData(void* node, int column_id) const { return {}; }
  virtual void HandleEditButton(void* node, int column_id) const {}

  virtual bool HasChildren(void* parent) const { return true; }

  virtual bool CanFetchMore(void* parent) const { return false; }
  virtual void FetchMore(void* parent) {}

 protected:
  void TreeNodesAdding(void* parent, int start, int count);
  void TreeNodesAdded(void* parent, int start, int count);
  void TreeNodesDeleting(void* parent, int start, int count);
  void TreeNodesDeleted(void* parent, int start, int count);
  void TreeNodeChanged(void* node);
  void TreeModelResetting();
  void TreeModelReset();

 private:
  boost::signals2::signal<void(void*, int, int)> nodes_adding_signal_;
  boost::signals2::signal<void(void*, int, int)> nodes_added_signal_;
  boost::signals2::signal<void(void*, int, int)> nodes_deleting_signal_;
  boost::signals2::signal<void(void*, int, int)> nodes_deleted_signal_;
  boost::signals2::signal<void(void*)> node_changed_signal_;
  boost::signals2::signal<void()> model_resetting_signal_;
  boost::signals2::signal<void()> model_reset_signal_;
};

}  // namespace aui
