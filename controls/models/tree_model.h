#pragma once

#include "controls/color.h"
#include "controls/models/edit_data.h"

#include <set>

namespace aui {

class TreeModelObserver;

class TreeModel {
 public:
  TreeModel() {}
  virtual ~TreeModel() {}

  TreeModel(const TreeModel&) = delete;
  TreeModel& operator=(const TreeModel&) = delete;

  void AddObserver(TreeModelObserver& observer) {
    observers_.insert(&observer);
  }
  void RemoveObserver(TreeModelObserver& observer) {
    observers_.erase(&observer);
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
    return ColorCode::Black;
  }
  virtual Color GetBackgroundColor(void* node, int column_id) {
    return ColorCode::Transparent;
  }

  virtual void SetText(void* node, int column_id, const std::u16string& text) {}
  virtual bool IsEditable(void* node, int column_id) const { return false; }
  virtual bool IsSelectable(void* node, int column_id) const { return true; }
  virtual EditData GetEditData(void* node, int column_id) { return {}; }

  virtual bool HasChildren(void* parent) const { return true; }

  virtual bool CanFetchMore(void* parent) const { return false; }
  virtual void FetchMore(void* parent) {}

 protected:
  typedef std::set<TreeModelObserver*> ObserverList;

  void TreeNodesAdding(void* parent, int start, int count);
  void TreeNodesAdded(void* parent, int start, int count);
  void TreeNodesDeleting(void* parent, int start, int count);
  void TreeNodesDeleted(void* parent, int start, int count);
  void TreeNodeChanged(void* node);
  void TreeModelResetting();
  void TreeModelReset();

  const ObserverList& observers() const { return observers_; }

 private:
  ObserverList observers_;
};

class TreeModelObserver {
 public:
  virtual void OnTreeNodesAdding(void* parent, int start, int count) {}
  virtual void OnTreeNodesAdded(void* parent, int start, int count) {}
  virtual void OnTreeNodesDeleting(void* parent, int start, int count) {}
  virtual void OnTreeNodesDeleted(void* parent, int start, int count) {}
  virtual void OnTreeNodeChanged(void* node) {}
  virtual void OnTreeModelResetting() {}
  virtual void OnTreeModelReset() {}
};

}  // namespace aui
