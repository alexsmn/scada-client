#pragma once

class DialogService;
template <class T>
class BasicCommandRegistry;
class UiCommandRegistry;
struct GlobalCommandContext;

void ShowAboutDialog(DialogService& dialog_service);

void RegisterAboutCommands(
    BasicCommandRegistry<GlobalCommandContext>& global_commands,
    UiCommandRegistry& ui_command_registry);
