#include "components/vidicon_display/vidicon_client.h"

VidiconClient* VidiconClient::s_instance = NULL;

VidiconClient::VidiconClient() {
  HRESULT res = ::CoCreateInstance(__uuidof(TeleClientLib::Client), nullptr,
                                   CLSCTX_ALL, IID_PPV_ARGS(&teleclient_));
  assert(SUCCEEDED(res));
}

VidiconClient::~VidiconClient() {}

// static
VidiconClient& VidiconClient::GetInstance() {
  if (!s_instance)
    s_instance = new VidiconClient;
  return *s_instance;
}

// static
void VidiconClient::CleanupInstance() {
  delete s_instance;
  s_instance = NULL;
}
