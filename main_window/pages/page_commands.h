#pragma once

#include "base/any_executor.h"

#include "base/awaitable.h"

#include <functional>
#include <memory>
#include <string>

class ActionManager;
class DialogService;
class MainWindowManager;
class Profile;
struct GlobalCommandContext;

using RenamePagePromptRunner =
    std::function<Awaitable<std::u16string>(DialogService& dialog_service,
                                            std::u16string current_title)>;

struct PageCommandsContext {
  AnyExecutor executor_;
  ActionManager& action_manager_;
  Profile& profile_;
  MainWindowManager& main_window_manager_;
  RenamePagePromptRunner rename_prompt_runner_;
};

class PageCommands : private PageCommandsContext {
 public:
  explicit PageCommands(PageCommandsContext&& context);

 private:
  void RenameCurrentPage(const GlobalCommandContext& context);
};
