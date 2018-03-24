#pragma once

#include "base/strings/string16.h"
#include "core/configuration_types.h"

#include <set>

class Portfolio {
 public:
  base::string16 name;

  typedef std::set<scada::NodeId> Items;
  Items items;
};
