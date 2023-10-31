#pragma once

#include "scada/node_id.h"

#include <set>
#include <string>

class Portfolio {
 public:
  std::u16string name;

  using Items = std::set<scada::NodeId>;
  Items items;
};
