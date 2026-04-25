#include "main_window/page_commands.h"

#include "aui/prompt_dialog.h"
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
    const GlobalCommandContext& context) {
  auto title = co_await AwaitPromise(
      NetExecutorAdapter{executor},
      RunPromptDialog(context.dialog_service, u"Имя:", u"Переименование",
                      context.main_window.GetCurrentPage().title));
  // TODO: Fix capture main_window.
  context.main_window.SetCurrentPageTitle(title);
  co_return;
}

}  // namespace

PageCommands::PageCommands(PageCommandsContext&& context)
    : PageCommandsContext{std::move(context)} {
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

promise<void> PageCommands::RenameCurrentPage(
    const GlobalCommandContext& context) {
  return ToPromise(NetExecutorAdapter{executor_},
                   RenameCurrentPageAsync(executor_, context));
}
