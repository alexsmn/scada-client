#pragma once

#include "components/multi_create/multi_create_dialog.h"

#include <map>

class MultiCreateModel : private MultiCreateContext {
 public:
  explicit MultiCreateModel(MultiCreateContext&& context);

  using Devices = std::map<std::wstring, scada::NodeId>;
  const Devices& devices() const { return devices_; }

  std::wstring GetAutoName(bool ts) const;

  struct RunParams {
    std::wstring device;
    int count = 0;
    bool ts = true;
    int starting_number = 1;
    int starting_address = 1;
    std::wstring name_prefix;
    std::string path_prefix;
  };

  void Run(const RunParams& params);

 private:
  Devices devices_;
};
