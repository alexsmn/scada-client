#pragma once

class DialogService;
template <class T>
class BasicCommandRegistry;
struct GlobalCommandContext;

void ShowAboutDialog(DialogService& dialog_service);

void RegisterAboutCommands(
    BasicCommandRegistry<GlobalCommandContext>& global_commands);
