#pragma once

#include "base/any_executor.h"

#include <memory>
#include <wrl/client.h>

class TimedDataService;

interface IClient;
using TeleClient = IClient;

namespace vidicon {

class ComDataPointManager;
class DataPointManager;

struct VidiconClientContext {
  AnyExecutor executor_;
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
