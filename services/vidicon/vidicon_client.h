#pragma once

#include "services/vidicon/teleclient.h"

#include <memory>
#include <wrl/client.h>

class Executor;
class TimedDataService;

namespace vidicon {

class ComDataPointManager;
class DataPointManager;

using TeleClient = TeleClientLib::IClient;

struct VidiconClientContext {
  std::shared_ptr<Executor> executor_;
  TimedDataService& timed_data_service_;
};

class VidiconClient : private VidiconClientContext {
 public:
  explicit VidiconClient(VidiconClientContext&& context);
  ~VidiconClient();

  TeleClient& teleclient() { return *teleclient_.Get(); }

 private:
  std::unique_ptr<ComDataPointManager> CreateComDataPointManager();

  std::unique_ptr<DataPointManager> data_point_manager_;
  std::unique_ptr<ComDataPointManager> com_data_point_manager_;

  Microsoft::WRL::ComPtr<TeleClient> teleclient_;
};

}  // namespace vidicon
