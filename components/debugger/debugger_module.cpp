#include "debugger_module.h"

#include "base/command_line.h"
#include "components/debugger/debug_switch.h"
#include "controller/command_registry.h"
#include "debugger.h"

#include <memory>

DebuggerModule::DebuggerModule(DebuggerModuleContext&& context)
    : DebuggerModuleContext{std::move(context)} {
  if (base::CommandLine::ForCurrentProcess()->HasSwitch(kDebugSwitch)) {
    auto debugger = std::make_shared<Debugger>(
        DebuggerContext{.session_service_ = session_service_});

    main_commands_.AddCommand(
        {.title = u"Отладчик",
         .menu_group = MenuGroup::DEBUG,
         .execute_handler = [debugger](const MainCommandContext& context) {
           debugger->Open();
         }});
  }
}
