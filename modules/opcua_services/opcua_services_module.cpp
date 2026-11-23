#include "modules/opcua_services/opcua_services_module.h"

#include "model/namespace_uris.h"
#include "opcua/client/client_session.h"
#include "opcua_bridge/client_adapters.h"
#include "scada/data_services_factory.h"
#include "services/user_node_session.h"

#include <memory>
#include <mutex>

namespace {

// Builds the OPC UA client backend: an opcuapp ClientSession presented to the
// SCADA client as core services through the scada<->opcua boundary adapter.
bool CreateOpcUaServices(const DataServicesContext& context,
                         DataServices& services) {
  auto session = std::make_shared<opcua::ClientSession>(
      context.executor, context.transport_factory);
  // Learn the server's namespace indexes from its published NamespaceArray and
  // remap NodeIds against the client's own canonical array, so the client works
  // against a tier that publishes only the namespaces it serves (ADR 0003). The
  // client keeps its compiled-in NamespaceIndexes:: constants; only the wire
  // boundary translates.
  services = scada::opcua_bridge::CreateRemappingClientDataServices(
      std::move(session), scada::model::GetCanonicalNamespaceUris());
  // The OPC UA session cannot name the account it signed in as, so the node
  // is looked up in the Users folder after connect -- without it the profile
  // never reaches the Server over OPC UA (backlog 1110).
  services.session_service_ = std::make_shared<UserNodeSessionService>(
      std::move(services.session_service_), services.view_service_);
  return true;
}

}  // namespace

OpcUaServicesModule::OpcUaServicesModule(OpcUaServicesModuleContext&& context)
    : OpcUaServicesModuleContext{std::move(context)} {
  static std::once_flag registered;
  std::call_once(registered, [] {
    RegisterDataServices(
        {"OpcUa", u"OPC UA", CreateOpcUaServices, "opc.tcp://localhost:4840"});
  });
}
