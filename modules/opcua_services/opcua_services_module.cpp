#include "modules/opcua_services/opcua_services_module.h"

#include "opcua/client_session.h"
#include "opcua_bridge/client_adapters.h"
#include "scada/data_services_factory.h"

#include <memory>
#include <mutex>

namespace {

// Builds the OPC UA client backend: an opcuapp ClientSession presented to the
// SCADA client as core services through the scada<->opcua boundary adapter.
bool CreateOpcUaServices(const DataServicesContext& context,
                         DataServices& services) {
  auto session = std::make_shared<opcua::ClientSession>(
      context.executor, context.transport_factory);
  services = opcua_bridge::CreateClientDataServices(std::move(session));
  return true;
}

}  // namespace

OpcUaServicesModule::OpcUaServicesModule(OpcUaServicesModuleContext&& context)
    : OpcUaServicesModuleContext{std::move(context)} {
  static std::once_flag registered;
  std::call_once(registered, [] {
    RegisterDataServices({"OpcUa", u"OPC UA", CreateOpcUaServices,
                          "opc.tcp://localhost:4840"});
  });
}
