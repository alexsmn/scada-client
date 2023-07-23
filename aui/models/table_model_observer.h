#pragma once

namespace aui {

class TableModelObserver {
 public:
  virtual ~TableModelObserver() = default;

  // Range has been changed.
  virtual void OnModelChanged() {}

  // Data in specified range has been changed.
  virtual void OnItemsChanged(int first, int count) {}

  virtual void OnItemsAdding(int first, int count) {}
  virtual void OnItemsAdded(int first, int count) {}
  virtual void OnItemsRemoving(int first, int count) {}
  virtual void OnItemsRemoved(int first, int count) {}
};

}  // namespace aui
