#include "components/vidicon_display/vidicon_client.h"

#include "components/vidicon_display/teleclient/com_data_point_manager.h"
#include "components/vidicon_display/teleclient/com_teleclient_impl.h"
#include "components/vidicon_display/teleclient/data_point_manager.h"

#include <cassert>

// #import
// "c:\TC\vidicon\vidicon2\build-vcpkg\src\TeleClient\teleclient.dir\RelWithDebInfo\TeleClient.tlb"
// raw_interfaces_only

class DummyAtlModule : public CAtlExeModuleT<DummyAtlModule> {};

DummyAtlModule _Module;

// VidiconClient

VidiconClient* VidiconClient::s_instance = nullptr;

VidiconClient::VidiconClient()
    : data_point_manager_{std::make_unique<DataPointManagerImpl>()},
      com_data_point_manager_{CreateComDataPointManager()} {
  auto* com_teleclient = new CComObjectNoLock<ComTeleclientImpl>();
  com_teleclient->Init(*com_data_point_manager_);
  teleclient_ = com_teleclient;
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
  s_instance = nullptr;
}

std::unique_ptr<ComDataPointManager>
VidiconClient::CreateComDataPointManager() {
  DataPointProvider data_point_provider =
      [&data_point_manager = *data_point_manager_](std::wstring_view address) {
        return data_point_manager.GetDataPoint(address);
      };
  return std::make_unique<ComDataPointManager>(std::move(data_point_provider));
}