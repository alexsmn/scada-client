#include "modules/about/about_dialog.h"

#include "aui/dialog_service.h"
#include "aui/translation.h"
#include "controller/command_registry.h"
#include "controller/command_ui_registry.h"
#include "core/global_command_context.h"
#include "main_window/standard_command_ids.h"
#include "modules/about/about_info.h"
#include "resources/common_resources.h"

#if defined(UI_QT)
#include <QMessageBox>
#endif

void RegisterAboutCommands(
    BasicCommandRegistry<GlobalCommandContext>& global_commands,
    UiCommandRegistry& ui_command_registry,
    scada::SessionService& session_service,
    NodeService& node_service) {
  global_commands.AddCommand(
      {.command_id = ID_APP_ABOUT,
       .title = Translate("About..."),
       .execute_handler = [&session_service,
                           &node_service](const GlobalCommandContext& context) {
         // Collected at the moment the dialog is opened, not once at startup:
         // the session rows are about the link as it is now, and the command
         // is reachable from the login window where there is no session yet.
         ShowAboutDialog(context.dialog_service,
                         CollectAboutInfo(session_service, node_service));
       }});
  ui_command_registry.AddMenuItem({.menu_id = MainMenuId::Help,
                                   .order = 900,
                                   .command_id = ID_APP_ABOUT,
                                   .separator_before = true});

#if defined(UI_QT)
  global_commands.AddCommand(
      {.command_id = ID_ABOUT_QT,
       .title = Translate("About Qt..."),
       .execute_handler = [](const GlobalCommandContext& context) {
         QMessageBox::aboutQt(context.dialog_service.GetParentWidget());
       }});
  ui_command_registry.AddMenuItem(
      {.menu_id = MainMenuId::Help, .order = 910, .command_id = ID_ABOUT_QT});
#endif
}
