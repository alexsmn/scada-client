#pragma once

#include "controller/node_id_set.h"

class ContentsObserver {
 public:
  virtual ~ContentsObserver() {}

  virtual void OnContentsChanged(const NodeIdSet& node_ids) = 0;
  virtual void OnContainedItemChanged(const scada::NodeId& node_id,
                                      bool added) = 0;
};
