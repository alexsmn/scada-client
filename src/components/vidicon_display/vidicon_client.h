#pragma once

#include "base/win/scoped_comptr.h"
#include "client/components/vidicon_display/teleclient.h"

class VidiconClient {
 public:
  typedef TeleClientLib::IClient TeleClient;

  TeleClient* GetTeleClient() { return teleclient_.get(); }

  void Shutdown();

  static VidiconClient& GetInstance();
  static void CleanupInstance();

 private:
  VidiconClient();
  ~VidiconClient();

  base::win::ScopedComPtr<TeleClient> teleclient_;

  static VidiconClient* s_instance;
};
