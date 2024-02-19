#pragma once

class DialogService;
class MainWindow;

struct MainCommandContext {
  MainWindow& main_window;
  DialogService& dialog_service;
};
