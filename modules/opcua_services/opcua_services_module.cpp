#include "modules/opcua_services/opcua_services_module.h"

#include <utility>

OpcUaServicesModule::OpcUaServicesModule(OpcUaServicesModuleContext&& context)
    : OpcUaServicesModuleContext{std::move(context)} {}
