#include "app/opcua_services_module.h"

#include "scada/data_services_factory.h"

#include <mutex>

namespace opcua {
bool CreateServices(const DataServicesContext& context, DataServices& services);
}

OpcUaServicesModule::OpcUaServicesModule(OpcUaServicesModuleContext&& context)
    : OpcUaServicesModuleContext{std::move(context)} {
  static std::once_flag registered;
  std::call_once(registered, [] {
    RegisterDataServices({"OpcUa", u"OPC UA", opcua::CreateServices,
                          "opc.tcp://localhost:4840"});
  });
}
