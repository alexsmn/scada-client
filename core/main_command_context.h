#pragma once

class DialogService;
class MainWindowInterface;

struct MainCommandContext {
  MainWindowInterface& main_window;
  DialogService& dialog_service;
};
