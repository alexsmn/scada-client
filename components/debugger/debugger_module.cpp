#include "debugger_module.h"

#include "controller/command_registry.h"
#include "debugger.h"

#include <memory>

DebuggerModule::DebuggerModule(DebuggerModuleContext&& context)
    : DebuggerModuleContext{std::move(context)} {
  auto debugger = std::make_shared<Debugger>(
      DebuggerContext{.session_service_ = session_service_});

  main_commands_.AddCommand(
      {.title = u"Отладчик",
       .menu_group = MenuGroup::DEBUG,
       .execute_handler = [debugger](const MainCommandContext& context) {
         debugger->Open();
       }});
}
