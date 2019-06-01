#pragma once

#include <functional>
#include <map>
#include <memory>

namespace base {
class FilePath;
}

class MainWindow;
class OpenedView;
class Page;
class Profile;

struct MainWindowManagerContext {
  Profile& profile_;
  const std::function<std::unique_ptr<MainWindow>(int window_id)>
      main_window_factory_;
  const std::function<void()> quit_handler_;
};

class MainWindowManager : private MainWindowManagerContext {
 public:
  explicit MainWindowManager(MainWindowManagerContext&& context);
  ~MainWindowManager();

  void Init();

  void CreateMainWindow();
  void OpenMainWindow(int window_id);
  void CloseMainWindow(int window_id);

  void OnMainWindowClosed(int window_id);

  typedef std::map<int /*window_id*/, std::unique_ptr<MainWindow>> MainWindows;
  const MainWindows& main_windows() const { return main_windows_; }

  bool IsPageOpened(int page_id) const;
  Page* FindFirstNotOpenedPage();
  OpenedView* FindOpenedViewByFilePath(const base::FilePath& path);

 private:
  MainWindows main_windows_;
};
