#pragma once

#include "base/any_executor.h"
#include "export/configuration/configuration_transfer_client.h"

#include <memory>

template <typename T>
class BasicCommandRegistry;
class NodeService;
class UiCommandRegistry;
struct GlobalCommandContext;

namespace scada {
class SessionService;
}

struct ExportConfigurationModuleContext {
  AnyExecutor executor_;
  // The session the transfer object is called through.
  NodeService& node_service_;
  BasicCommandRegistry<GlobalCommandContext>& global_commands_;
  UiCommandRegistry& ui_command_registry_;
  // Calls to use instead of the session's, for tests.
  ConfigurationTransferCall call_;
  // Optional: asked whether the server is ours. The transfer object exists
  // only on our configuration server, so both commands are withheld on any
  // other OPC UA server rather than failing as "too old". Null offers them.
  scada::SessionService* session_service_ = nullptr;
};

// "Export Configuration..." and "Import Configuration..." for administrators:
// the whole configuration as a UANodeSet file, moved over the configuration
// server's transfer object (ADR 0014). See configuration_transfer_commands.h.
class ExportConfigurationModule : private ExportConfigurationModuleContext {
 public:
  explicit ExportConfigurationModule(
      ExportConfigurationModuleContext&& context);

 private:
  std::shared_ptr<const ConfigurationTransferClient> client_;
};
