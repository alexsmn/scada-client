#pragma once

#include "core/configuration_types.h"

#include <filesystem>

namespace htsde2 {
struct IHTSDEForm2;
}

class ModusViewWrapper {
 public:
  virtual ~ModusViewWrapper() {}

  virtual void Open(const std::filesystem::path& path) = 0;

  virtual std::filesystem::path GetPath() const = 0;

  virtual bool ShowContainedItem(const scada::NodeId& item_id) = 0;

  // TODO: Factor out.
  virtual htsde2::IHTSDEForm2* GetSdeForm() = 0;
};
