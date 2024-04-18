#pragma once

#include <filesystem>
#include <functional>
#include <map>
#include <memory>
#include <ranges>

class MainWindow;
class OpenedView;
class Page;
class Profile;

using MainWindowFactory =
    std::function<std::unique_ptr<MainWindow>(int window_id)>;

using QuitHandler = std::function<void()>;

struct MainWindowManagerContext {
  Profile& profile_;
  const MainWindowFactory main_window_factory_;
  const QuitHandler quit_handler_;
};

class MainWindowManager : private MainWindowManagerContext {
 public:
  explicit MainWindowManager(MainWindowManagerContext&& context);
  ~MainWindowManager();

  void Init();

  MainWindow* CreateMainWindow();
  MainWindow* OpenMainWindow(int window_id);
  void CloseMainWindow(int window_id);

  void OnMainWindowClosed(int window_id);

  auto main_windows() {
    return main_windows_ | std::views::values |
           std::views::transform(
               [](const std::unique_ptr<MainWindow>& ptr) -> MainWindow& {
                 return *ptr;
               });
  }

  bool IsPageOpened(int page_id) const;
  Page* FindFirstNotOpenedPage();
  OpenedView* FindOpenedViewByFilePath(const std::filesystem::path& path);

 private:
  std::map<int /*window_id*/, std::unique_ptr<MainWindow>> main_windows_;
};
