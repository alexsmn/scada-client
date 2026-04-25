#include "main_window/page_commands.h"

#include "aui/prompt_dialog.h"
#include "base/awaitable.h"
#include "base/awaitable_promise.h"
#include "resources/common_resources.h"
#include "controller/command_registry.h"
#include "core/global_command_context.h"
#include "main_window/initial_page.h"
#include "main_window/main_window_interface.h"
#include "main_window/main_window_manager.h"
#include "net/net_executor_adapter.h"
#include "profile/profile.h"

namespace {

Awaitable<void> RenameCurrentPageAsync(
    std::shared_ptr<Executor> executor,
    MainWindowInterface& main_window,
    DialogService& dialog_service,
    RenamePagePromptRunner prompt_runner,
    std::u16string current_page_title) {
  auto title = co_await AwaitPromise(
      NetExecutorAdapter{executor},
      prompt_runner(dialog_service, std::move(current_page_title)));
  main_window.SetCurrentPageTitle(title);
  co_return;
}

}  // namespace

PageCommands::PageCommands(PageCommandsContext&& context)
    : PageCommandsContext{std::move(context)} {
  if (!rename_prompt_runner_) {
    rename_prompt_runner_ = [](DialogService& dialog_service,
                               std::u16string current_title) {
      return RunPromptDialog(dialog_service, u"Имя:", u"Переименование",
                             current_title);
    };
  }

  global_commands_.AddCommand(
      {.command_id = ID_PAGE_NEW,
       .execute_handler = [this](const GlobalCommandContext& context) {
         context.main_window.SaveCurrentPage();
         auto& page = profile_.AddPage(CreateInitialPage());
         context.main_window.OpenPage(page);
       }});

  global_commands_.AddCommand(
      {.command_id = ID_PAGE_RENAME,
       .execute_handler = [this](const GlobalCommandContext& context) {
         RenameCurrentPage(context);
       }});

  global_commands_.AddCommand(
      {.command_id = ID_PAGE_DELETE,
       .execute_handler = [this](const GlobalCommandContext& context) {
         context.main_window.DeleteCurrentPage();
       }});
}

void PageCommands::RenameCurrentPage(const GlobalCommandContext& context) {
  CoSpawn(executor_,
          [executor = executor_, &main_window = context.main_window,
           &dialog_service = context.dialog_service,
           prompt_runner = rename_prompt_runner_,
           current_page_title = context.main_window.GetCurrentPage().title] {
            return RenameCurrentPageAsync(executor, main_window, dialog_service,
                                          std::move(prompt_runner),
                                          current_page_title);
          });
}
