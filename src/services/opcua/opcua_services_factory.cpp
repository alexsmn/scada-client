#include "services/data_services_factory.h"
#include "services/opcua/opcua_session.h"

DataServices CreateOpcUaServices(const DataServicesContext& context) {
  auto session = std::make_shared<OpcUaSession>();
  return {
      session,
      session,
      nullptr,
      nullptr,
      nullptr,
      session,
      nullptr,
      session,
  };
}
