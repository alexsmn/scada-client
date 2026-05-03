#pragma once

struct OpcUaServicesModuleContext {
};

class OpcUaServicesModule : private OpcUaServicesModuleContext {
 public:
  explicit OpcUaServicesModule(OpcUaServicesModuleContext&& context = {});
};
