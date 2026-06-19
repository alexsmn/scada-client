#include "modules/about/about_dialog.h"

#include "aui/dialog_service.h"
#include "aui/translation.h"
#include "controller/command_registry.h"
#include "controller/command_ui_registry.h"
#include "core/global_command_context.h"
#include "main_window/standard_command_ids.h"
#include "resources/common_resources.h"

#if defined(UI_QT)
#include <QMessageBox>
#endif

void RegisterAboutCommands(
    BasicCommandRegistry<GlobalCommandContext>& global_commands,
    UiCommandRegistry& ui_command_registry) {
  global_commands.AddCommand(
      {.command_id = ID_APP_ABOUT,
       .title = Translate("About..."),
       .execute_handler = [](const GlobalCommandContext& context) {
         ShowAboutDialog(context.dialog_service);
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
