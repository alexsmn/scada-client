#pragma once

struct MainWindowModuleContext {};

class MainWindowModule : private MainWindowModuleContext {
 public:
  explicit MainWindowModule(MainWindowModuleContext&& context);
  ~MainWindowModule();
};