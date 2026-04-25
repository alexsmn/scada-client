#pragma once

#include "base/promise.h"

#include <functional>
#include <memory>
#include <string>

template <class T>
class BasicCommandRegistry;

class DialogService;
class Executor;
class MainWindowManager;
class Profile;
struct GlobalCommandContext;

using RenamePagePromptRunner =
    std::function<promise<std::u16string>(DialogService& dialog_service,
                                          std::u16string current_title)>;

struct PageCommandsContext {
  std::shared_ptr<Executor> executor_;
  BasicCommandRegistry<GlobalCommandContext>& global_commands_;
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
