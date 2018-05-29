#pragma once

#include "components/vidicon_display/teleclient.h"

#include <wrl/client.h>

class VidiconClient {
 public:
  typedef TeleClientLib::IClient TeleClient;

  TeleClient* GetTeleClient() { return teleclient_.Get(); }

  static VidiconClient& GetInstance();
  static void CleanupInstance();

 private:
  VidiconClient();
  ~VidiconClient();

  Microsoft::WRL::ComPtr<TeleClient> teleclient_;

  static VidiconClient* s_instance;
};
