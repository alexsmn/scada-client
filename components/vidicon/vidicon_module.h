#pragma once

#include "base/any_executor.h"

#include <memory>

namespace vidicon {
class VidiconClient;
}

class ControllerRegistry;
class FileRegistry;
class TimedDataService;
class WriteService;

struct VidiconModuleContext {
  AnyExecutor executor_;
  TimedDataService& timed_data_service_;
  ControllerRegistry& controller_registry_;
  WriteService& write_service_;
  FileRegistry& file_registry_;
};

class VidiconModule : private VidiconModuleContext {
 public:
  explicit VidiconModule(VidiconModuleContext&& context);
  ~VidiconModule();

 private:
  std::unique_ptr<vidicon::VidiconClient> vidicon_client_;
};
