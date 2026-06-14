#pragma once

#include "base/any_executor.h"
#include "base/awaitable.h"
#include "profile/window_definition.h"

#include <optional>
#include <span>

template <class T>
class BasicCommandRegistry;

class NodeRef;
class UiCommandRegistry;
struct SelectionCommandContext;

struct DeviceMetricsModuleContext {
  AnyExecutor executor_;
  BasicCommandRegistry<SelectionCommandContext>& selection_commands_;
  UiCommandRegistry& ui_command_registry_;
};

class DeviceMetricsModule : private DeviceMetricsModuleContext {
 public:
  explicit DeviceMetricsModule(DeviceMetricsModuleContext&& context);
};

WindowDefinition MakeDeviceMetricsWindowDefinitionSync(
    std::u16string title,
    std::span<const NodeRef> devices);

Awaitable<WindowDefinition> MakeDeviceMetricsWindowDefinitionAsync(
    AnyExecutor executor,
    const NodeRef& device);
