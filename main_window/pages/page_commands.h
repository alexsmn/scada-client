#pragma once

#include "base/any_executor.h"

#include "base/awaitable.h"

#include <functional>
#include <memory>
#include <string>

template <class T>
class BasicCommandRegistry;

class DialogService;
class MainWindowManager;
class Profile;
class UiCommandRegistry;
struct GlobalCommandContext;

using RenamePagePromptRunner =
    std::function<Awaitable<std::u16string>(DialogService& dialog_service,
                                            std::u16string current_title)>;

struct PageCommandsContext {
  AnyExecutor executor_;
  BasicCommandRegistry<GlobalCommandContext>& global_commands_;
  UiCommandRegistry& ui_command_registry_;
  Profile& profile_;
  MainWindowManager& main_window_manager_;
  RenamePagePromptRunner rename_prompt_runner_;
};

class PageCommands : private PageCommandsContext {
 public:
  explicit PageCommands(PageCommandsContext&& context);

 private:
  void RenameCurrentPage(const GlobalCommandContext& context);
  // Copies the current page — layout, windows and icon — into a new one and
  // opens it. The copy is taken after saving, so it carries what is on screen
  // rather than what was last persisted.
  void DuplicateCurrentPage(const GlobalCommandContext& context);
};
