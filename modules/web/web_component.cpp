#include "modules/web/web_component.h"

#include "aui/translation.h"
#include "base/awaitable.h"
#include "controller/command_registry.h"
#include "controller/controller_registry.h"
#include "core/global_command_context.h"
#include "main_window/main_window_interface.h"
#include "modules/web/web_view.h"
#include "net/net_executor_adapter.h"
#include "profile/window_definition.h"

#include <filesystem>

REGISTER_CONTROLLER(WebView, kWebWindowInfo);

void RegisterWebCommands(
    AnyExecutor executor,
    BasicCommandRegistry<GlobalCommandContext>& global_commands) {
#if defined(_WIN32) && !defined(UI_WT)
  global_commands.AddCommand(
      {.command_id = ID_HELP_MANUAL,
       .execute_handler = [executor = std::move(executor)](
                              const GlobalCommandContext& context) {
         WindowDefinition def(kWebWindowInfo);
         def.title = Translate("Documentation");
         def.path = std::filesystem::path(
             L"http://www.telecontrol.ru/workplace_manual");
         CoSpawn(executor,
                 [&main_window = context.main_window,
                  def = std::move(def)]() -> Awaitable<void> {
                   co_await main_window.OpenView(def, true);
                 });
       }});
#else
  (void)executor;
  (void)global_commands;
#endif
}
