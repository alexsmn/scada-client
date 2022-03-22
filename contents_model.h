#pragma once

#include "common/node_state.h"
#include "contents_observer.h"
#include "controls/types.h"

class ContentsObserver;

class ContentsModel {
 public:
  virtual ~ContentsModel() {}

  // AddContainedItem() flags.
  enum { APPEND = 0x0001 };

  virtual void AddContainedItem(const scada::NodeId& node_id, unsigned flags) {}

  virtual void RemoveContainedItem(const scada::NodeId& node_id) {}

  virtual NodeIdSet GetContainedItems() const { return {}; }

  ContentsObserver* contents_observer = nullptr;

 protected:
  void NotifyContentsChanged(const NodeIdSet& contents);
  void NotifyContainedItemChanged(const scada::NodeId& item_id, bool added);
};

inline void ContentsModel::NotifyContentsChanged(const NodeIdSet& contents) {
  if (contents_observer)
    contents_observer->OnContentsChanged(contents);
}

inline void ContentsModel::NotifyContainedItemChanged(
    const scada::NodeId& node_id,
    bool added) {
  if (contents_observer)
    contents_observer->OnContainedItemChanged(node_id, added);
}
