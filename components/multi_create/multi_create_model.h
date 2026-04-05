#pragma once

#include "components/multi_create/multi_create_dialog.h"

#include <map>

class MultiCreateModel : private MultiCreateContext {
 public:
  explicit MultiCreateModel(MultiCreateContext&& context);

  using Devices = std::map<std::u16string, scada::NodeId>;
  const Devices& devices() const { return devices_; }

  std::u16string GetAutoName(bool ts) const;

  struct RunParams {
    std::u16string device;
    int count = 0;
    bool ts = true;
    int starting_number = 1;
    int starting_address = 1;
    std::u16string name_prefix;
    std::string path_prefix;
  };

  void Run(const RunParams& params);

 private:
  Devices devices_;
};
