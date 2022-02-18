#pragma once

#include "base/strings/string16.h"
#include "core/configuration_types.h"

#include <set>
#include <string>

class Portfolio {
 public:
  std::u16string name;

  typedef std::set<scada::NodeId> Items;
  Items items;
};
