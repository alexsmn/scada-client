#pragma once

#include "base/any_executor.h"
#include "controller/window_info.h"
#include "scada/session_service.h"

template <class T>
class BasicCommandRegistry;

class UiCommandRegistry;
struct SelectionCommandContext;

extern const WindowInfo kTransmissionWindowInfo;

struct TransmissionModuleContext {
  AnyExecutor executor_;
  scada::SessionService& session_service_;
  BasicCommandRegistry<SelectionCommandContext>& selection_commands_;
  UiCommandRegistry& ui_command_registry_;
};

class TransmissionModule : private TransmissionModuleContext {
 public:
  explicit TransmissionModule(TransmissionModuleContext&& context);
};
