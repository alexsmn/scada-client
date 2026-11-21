#include "main_window/pages/page_commands.h"

#include "aui/dialog_service.h"
#include "aui/prompt_dialog.h"
#include "aui/translation.h"
#include "base/awaitable.h"
#include "base/u16format.h"
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

// Asks before deleting the current page, which used to go on one click
// (backlog 924). Its windows go to the trash, as a closed window does.
Awaitable<void> DeleteCurrentPageAsync(MainWindowInterface& main_window,
                                       DialogService& dialog_service,
                                       std::u16string page_title) {
  const MessageBoxResult answer = co_await dialog_service.RunMessageBox(
      u16format(Translate("Delete the page \"{}\"? Its windows can be reopened "
                          "from the trash."),
                page_title),
      Translate("Delete Page"), MessageBoxMode::QuestionYesNo);
  if (answer != MessageBoxResult::Yes)
    co_return;
  // Deleting closes the page's views, and staged edits with them (backlog
  // 1077); a Cancel there keeps the page.
  if (!co_await main_window.ResolvePendingChanges())
    co_return;
  main_window.DeleteCurrentPage();
}

// Opening a new page closes the current one's views, so their staged edits are
// settled first (backlog 1077).
Awaitable<void> NewPageAsync(MainWindowInterface& main_window,
                             Profile& profile) {
  if (!co_await main_window.ResolvePendingChanges())
    co_return;
  main_window.SaveCurrentPage();
  Page& page = profile.AddPage(CreateInitialPage());
  main_window.OpenPage(page);
}

// As NewPageAsync, with a copy of the current page.
Awaitable<void> DuplicateCurrentPageAsync(MainWindowInterface& main_window,
                                          Profile& profile) {
  if (!co_await main_window.ResolvePendingChanges())
    co_return;

  // Save first, so the copy carries what is on screen rather than what was
  // last persisted — duplicating a page the operator has just rearranged and
  // getting the old layout would be indistinguishable from a bug.
  main_window.SaveCurrentPage();

  Page copy = main_window.GetCurrentPage();
  // AddPage assigns the id and the order; clearing the id here keeps the copy
  // from looking briefly like the original to anything reading it in between.
  copy.id = 0;
  copy.order = 0;
  copy.title =
      main_window.GetCurrentPage().GetTitle() + u" — " + Translate("copy");

  Page& duplicate = profile.AddPage(copy);
  main_window.OpenPage(duplicate);
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
         CoSpawn(executor_,
                 [&main_window = context.main_window, &profile = profile_] {
                   return NewPageAsync(main_window, profile);
                 });
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
         CoSpawn(executor_,
                 [&main_window = context.main_window,
                  &dialog_service = context.dialog_service,
                  title = context.main_window.GetCurrentPage().GetTitle()] {
                   return DeleteCurrentPageAsync(main_window, dialog_service,
                                                 title);
                 });
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
  // Last, behind a separator, as the page button's menu has it: the one
  // destructive command should not sit second, directly under «Новая»
  // (backlog 935).
  ui_command_registry_.AddMenuItem({.menu_id = MainMenuId::Page,
                                    .order = 900,
                                    .command_id = ID_PAGE_DELETE,
                                    .separator_before = true});
  ui_command_registry_.AddMenuItem({.menu_id = MainMenuId::Page,
                                    .order = 300,
                                    .command_id = ID_PAGE_RENAME});
}

void PageCommands::DuplicateCurrentPage(const GlobalCommandContext& context) {
  CoSpawn(executor_, [&main_window = context.main_window, &profile = profile_] {
    return DuplicateCurrentPageAsync(main_window, profile);
  });
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
