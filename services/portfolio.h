#pragma once

#include "core/configuration_types.h"

#include <set>
#include <string>

class Portfolio {
 public:
  std::wstring name;

  typedef std::set<scada::NodeId> Items;
  Items items;
};
