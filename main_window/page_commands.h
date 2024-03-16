#pragma once

#include "base/promise.h"

template <class T>
class BasicCommandRegistry;

class MainWindowManager;
class Profile;
struct GlobalCommandContext;

struct PageCommandsContext {
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