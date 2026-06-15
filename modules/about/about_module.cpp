#include "modules/about/about_dialog.h"

#include "aui/dialog_service.h"
#include "controller/command_registry.h"
#include "core/global_command_context.h"
#include "main_window/standard_command_ids.h"
#include "resources/common_resources.h"

#if defined(UI_QT)
#include <QMessageBox>
#endif

void RegisterAboutCommands(
    BasicCommandRegistry<GlobalCommandContext>& global_commands) {
  global_commands.AddCommand(
      {.command_id = ID_APP_ABOUT,
       .execute_handler = [](const GlobalCommandContext& context) {
         ShowAboutDialog(context.dialog_service);
       }});

#if defined(UI_QT)
  global_commands.AddCommand(
      {.command_id = ID_ABOUT_QT,
       .execute_handler = [](const GlobalCommandContext& context) {
         QMessageBox::aboutQt(context.dialog_service.GetParentWidget());
       }});
#endif
}
