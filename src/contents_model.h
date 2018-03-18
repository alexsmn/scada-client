#pragma once

#include "core/configuration_types.h"
#include "controls/types.h"

class ContentsObserver;

class ContentsModel {
 public:
  virtual ~ContentsModel() {}

  void set_contents_observer(ContentsObserver* observer) {
    contents_observer_ = observer;
  }

  // AddContainedItem() flags.
  enum { APPEND = 0x0001 };
  virtual void AddContainedItem(const scada::NodeId& node_id, unsigned flags) {}
  virtual void RemoveContainedItem(const scada::NodeId& node_id) {}

  virtual NodeIdSet GetContainedItems() const { return NodeIdSet(); }

 protected:
  ContentsObserver* contents_observer() { return contents_observer_; }

 private:
  ContentsObserver* contents_observer_ = nullptr;
};
