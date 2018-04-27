#pragma once

#include "base/files/file_path.h"
#include "core/configuration_types.h"

namespace htsde2 {
struct IHTSDEForm2;
}

class ModusViewWrapper {
 public:
  virtual ~ModusViewWrapper() {}

  virtual void Open(const base::FilePath& path) = 0;

  virtual base::FilePath GetPath() const = 0;

  virtual bool ShowContainedItem(const scada::NodeId& item_id) = 0;

  // TODO: Factor out.
  virtual htsde2::IHTSDEForm2* GetSdeForm() = 0;
};
