#include "main_window/page_commands.h"

#include "aui/prompt_dialog.h"
#include "common_resources.h"
#include "controller/command_registry.h"
#include "core/main_command_context.h"
#include "main_window/main_window_interface.h"
#include "main_window/main_window_manager.h"
#include "profile/profile.h"

PageCommands::PageCommands(PageCommandsContext&& context)
    : PageCommandsContext{std::move(context)} {
  main_commands_.AddCommand(
      {.command_id = ID_PAGE_NEW,
       .execute_handler = [this](const MainCommandContext& context) {
         context.main_window.SaveCurrentPage();
         auto& page = profile_.CreatePage();
         context.main_window.OpenPage(page);
       }});

  main_commands_.AddCommand(
      {.command_id = ID_PAGE_RENAME,
       .execute_handler = [this](const MainCommandContext& context) {
         RenameCurrentPage(context);
       }});

  main_commands_.AddCommand(
      {.command_id = ID_PAGE_DELETE,
       .execute_handler = [this](const MainCommandContext& context) {
         context.main_window.DeleteCurrentPage();
       }});
}

promise<void> PageCommands::RenameCurrentPage(
    const MainCommandContext& context) {
  return RunPromptDialog(context.dialog_service, u"Имя:", u"Переименование",
                         context.main_window.GetCurrentPage().title)
      // TODO: Fix capture main_window.
      .then(std::bind_front(&MainWindowInterface::SetCurrentPageTitle,
                            std::ref(context.main_window)));
}
