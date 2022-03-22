#pragma once

#include "base/strings/string16.h"
#include "common/node_state.h"

#include <set>
#include <string>

class Portfolio {
 public:
  std::u16string name;

  typedef std::set<scada::NodeId> Items;
  Items items;
};
