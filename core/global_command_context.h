#pragma once

class DialogService;
class MainWindowInterface;

struct GlobalCommandContext {
  MainWindowInterface& main_window;
  DialogService& dialog_service;
};
