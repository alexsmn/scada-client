#pragma once

#include "aui/types.h"
#include "common/node_state.h"
#include "controller/node_id_set.h"

#include <functional>

class ContentsModel {
 public:
  virtual ~ContentsModel() {}

  // AddContainedItem() flags.
  enum { APPEND = 0x0001 };

  virtual void AddContainedItem(const scada::NodeId& node_id, unsigned flags) {}

  virtual void RemoveContainedItem(const scada::NodeId& node_id) {}

  virtual NodeIdSet GetContainedItems() const { return {}; }

  // Set by the active main window; single consumer.
  std::function<void(const NodeIdSet& contents)> contents_changed_handler;
  std::function<void(const scada::NodeId& item_id, bool added)>
      contained_item_changed_handler;

 protected:
  void NotifyContentsChanged(const NodeIdSet& contents);
  void NotifyContainedItemChanged(const scada::NodeId& item_id, bool added);
};

inline void ContentsModel::NotifyContentsChanged(const NodeIdSet& contents) {
  if (contents_changed_handler)
    contents_changed_handler(contents);
}

inline void ContentsModel::NotifyContainedItemChanged(
    const scada::NodeId& node_id,
    bool added) {
  if (contained_item_changed_handler)
    contained_item_changed_handler(node_id, added);
}
