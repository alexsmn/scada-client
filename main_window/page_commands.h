#pragma once

#include "base/promise.h"

template <class T>
class BasicCommandRegistry;

class DialogService;
class MainWindowInterface;
class MainWindowManager;
class Profile;
struct MainCommandContext;

struct PageCommandsContext {
  BasicCommandRegistry<MainCommandContext>& main_commands_;
  Profile& profile_;
  MainWindowManager& main_window_manager_;
};

class PageCommands : private PageCommandsContext {
 public:
  explicit PageCommands(PageCommandsContext&& context);

 private:
  promise<void> RenameCurrentPage(const MainCommandContext& context);
};