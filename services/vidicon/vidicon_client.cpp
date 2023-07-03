#include "services/vidicon/vidicon_client.h"

#include "services/vidicon/com_data_point_manager.h"
#include "services/vidicon/com_teleclient_impl.h"
#include "services/vidicon/data_point_manager_impl.h"

#include <cassert>

// #import
// "c:\TC\vidicon\vidicon2\build-vcpkg\src\TeleClient\teleclient.dir\RelWithDebInfo\TeleClient.tlb"
// raw_interfaces_only

class DummyAtlModule : public CAtlExeModuleT<DummyAtlModule> {};

DummyAtlModule _Module;

namespace vidicon {

// VidiconClient

VidiconClient::VidiconClient(VidiconClientContext&& context)
    : VidiconClientContext{std::move(context)},
      data_point_manager_{
          std::make_unique<DataPointManagerImpl>(executor_,
                                                 timed_data_service_)},
      com_data_point_manager_{CreateComDataPointManager()} {
  teleclient_ = CreateComTeleClient(*com_data_point_manager_);
}

VidiconClient::~VidiconClient() = default;

std::unique_ptr<ComDataPointManager>
VidiconClient::CreateComDataPointManager() {
  return std::make_unique<ComDataPointManager>(*data_point_manager_);
}

}  // namespace vidicon
