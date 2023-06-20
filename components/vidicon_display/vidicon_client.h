#pragma once

#include "components/vidicon_display/teleclient/teleclient.h"

#include <memory>
#include <wrl/client.h>

class ComDataPointManager;
class DataPointManager;

class VidiconClient {
 public:
  typedef TeleClientLib::IClient TeleClient;

  TeleClient* GetTeleClient() { return teleclient_.Get(); }

  static VidiconClient& GetInstance();
  static void CleanupInstance();

 private:
  VidiconClient();
  ~VidiconClient();

  std::unique_ptr<ComDataPointManager> CreateComDataPointManager();

  std::unique_ptr<DataPointManager> data_point_manager_;
  std::unique_ptr<ComDataPointManager> com_data_point_manager_;

  Microsoft::WRL::ComPtr<TeleClient> teleclient_;

  static VidiconClient* s_instance;
};
