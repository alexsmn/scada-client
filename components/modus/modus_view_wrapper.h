#pragma once

#include "common/node_state.h"

#include <filesystem>

class ModusViewWrapper {
 public:
  virtual ~ModusViewWrapper() {}

  virtual void Open(const std::filesystem::path& path) = 0;

  virtual std::filesystem::path GetPath() const = 0;

  virtual bool ShowContainedItem(const scada::NodeId& item_id) = 0;
};
