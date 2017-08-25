#pragma once

#include "core/configuration_types.h"

#include <set>

class ContentsObserver {
 public:
  virtual void OnContainedItemsUpdate(const std::set<scada::NodeId>& item_ids) = 0;
  virtual void OnContainedItemChanged(const scada::NodeId& item_id, bool added) = 0;
};
