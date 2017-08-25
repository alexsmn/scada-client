#include "client/components/vidicon_display/vidicon_client.h"

#include "base/logging.h"

VidiconClient* VidiconClient::s_instance = NULL;

VidiconClient::VidiconClient() {
  HRESULT res = teleclient_.CreateInstance(__uuidof(TeleClientLib::Client));
  DCHECK(SUCCEEDED(res));
}

VidiconClient::~VidiconClient() {
}

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
