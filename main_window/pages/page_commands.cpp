#include "main_window/pages/page_commands.h"

#include "aui/prompt_dialog.h"
#include "aui/translation.h"
#include "base/awaitable.h"
#include "controller/command_registry.h"
#include "controller/command_ui_registry.h"
#include "core/global_command_context.h"
#include "main_window/main_window_interface.h"
#include "main_window/main_window_manager.h"
#include "main_window/pages/initial_page.h"
#include "net/net_executor_adapter.h"
#include "profile/profile.h"
#include "resources/common_resources.h"

namespace {

Awaitable<void> RenameCurrentPageAsync(AnyExecutor executor,
                                       MainWindowInterface& main_window,
                                       DialogService& dialog_service,
                                       RenamePagePromptRunner prompt_runner,
                                       std::u16string current_page_title) {
  auto title =
      co_await prompt_runner(dialog_service, std::move(current_page_title));
  main_window.SetCurrentPageTitle(title);
  co_return;
}

}  // namespace

PageCommands::PageCommands(PageCommandsContext&& context)
    : PageCommandsContext{std::move(context)} {
  if (!rename_prompt_runner_) {
    rename_prompt_runner_ = [](DialogService& dialog_service,
                               std::u16string current_title) {
      return RunPromptDialog(dialog_service, Translate("Name:"),
                             Translate("Rename Page"), current_title);
    };
  }

  global_commands_.AddCommand(
      {.command_id = ID_PAGE_NEW,
       .title = Translate("New"),
       .execute_handler = [this](const GlobalCommandContext& context) {
         context.main_window.SaveCurrentPage();
         auto& page = profile_.AddPage(CreateInitialPage());
         context.main_window.OpenPage(page);
       }});

  global_commands_.AddCommand(
      {.command_id = ID_PAGE_RENAME,
       .title = Translate("Rename"),
       .execute_handler = [this](const GlobalCommandContext& context) {
         RenameCurrentPage(context);
       }});

  global_commands_.AddCommand(
      {.command_id = ID_PAGE_DELETE,
       .title = Translate("Delete"),
       .execute_handler = [this](const GlobalCommandContext& context) {
         context.main_window.DeleteCurrentPage();
       }});

  global_commands_.AddCommand(
      {.command_id = ID_PAGE_DUPLICATE,
       .title = Translate("Duplicate"),
       .execute_handler = [this](const GlobalCommandContext& context) {
         DuplicateCurrentPage(context);
       }});

  ui_command_registry_.AddMenuItem({.menu_id = MainMenuId::Page,
                                    .order = 250,
                                    .command_id = ID_PAGE_DUPLICATE});

  ui_command_registry_.AddMenuItem(
      {.menu_id = MainMenuId::Page, .order = 100, .command_id = ID_PAGE_NEW});
  ui_command_registry_.AddMenuItem({.menu_id = MainMenuId::Page,
                                    .order = 200,
                                    .command_id = ID_PAGE_DELETE});
  ui_command_registry_.AddMenuItem({.menu_id = MainMenuId::Page,
                                    .order = 300,
                                    .command_id = ID_PAGE_RENAME});
}

void PageCommands::DuplicateCurrentPage(const GlobalCommandContext& context) {
  // Save first, so the copy carries what is on screen rather than what was last
  // persisted — duplicating a page the operator has just rearranged and getting
  // the old layout would be indistinguishable from a bug.
  context.main_window.SaveCurrentPage();

  Page copy = context.main_window.GetCurrentPage();
  // AddPage assigns the id and the order; clearing the id here keeps the copy
  // from looking briefly like the original to anything reading it in between.
  copy.id = 0;
  copy.order = 0;
  copy.title = context.main_window.GetCurrentPage().GetTitle() + u" — " +
               Translate("copy");

  Page& duplicate = profile_.AddPage(copy);
  context.main_window.OpenPage(duplicate);
}

void PageCommands::RenameCurrentPage(const GlobalCommandContext& context) {
  CoSpawn(executor_, [executor = executor_, &main_window = context.main_window,
                      &dialog_service = context.dialog_service,
                      prompt_runner = rename_prompt_runner_,
                      current_page_title =
                          context.main_window.GetCurrentPage().title] {
    return RenameCurrentPageAsync(executor, main_window, dialog_service,
                                  std::move(prompt_runner), current_page_title);
  });
}
