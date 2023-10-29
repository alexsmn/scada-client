#pragma once

#include <memory>

namespace vidicon {
class VidiconClient;
}

class Executor;
class ControllerRegistry;
class TimedDataService;
class WriteService;

struct VidiconModuleContext {
  std::shared_ptr<Executor> executor_;
  TimedDataService& timed_data_service_;
  ControllerRegistry& controller_registry_;
  WriteService& write_service_;
};

class VidiconModule : private VidiconModuleContext {
 public:
  explicit VidiconModule(VidiconModuleContext&& context);
  ~VidiconModule();

 private:
  std::unique_ptr<vidicon::VidiconClient> vidicon_client_;
};
