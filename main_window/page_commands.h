#pragma once

#include "base/promise.h"

#include <memory>

template <class T>
class BasicCommandRegistry;

class Executor;
class MainWindowManager;
class Profile;
struct GlobalCommandContext;

struct PageCommandsContext {
  std::shared_ptr<Executor> executor_;
  BasicCommandRegistry<GlobalCommandContext>& global_commands_;
  Profile& profile_;
  MainWindowManager& main_window_manager_;
};

class PageCommands : private PageCommandsContext {
 public:
  explicit PageCommands(PageCommandsContext&& context);

 private:
  promise<void> RenameCurrentPage(const GlobalCommandContext& context);
};
