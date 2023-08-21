#pragma once

#include "scada/node_id.h"

#include <filesystem>

class WindowDefinition;

class ModusViewWrapper {
 public:
  virtual ~ModusViewWrapper() = default;

  virtual void Open(const WindowDefinition& definition) = 0;

  virtual void Save(WindowDefinition& definition) = 0;

  virtual std::filesystem::path GetPath() const = 0;

  virtual bool ShowContainedItem(const scada::NodeId& item_id) = 0;
};
