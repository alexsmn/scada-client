#pragma once

#include "aui/types.h"
#include "base/executor_timer.h"
#include "controller/command_handler.h"
#include "controller/controller_delegate.h"
#include "controller/controller_factory.h"
#include "main_window/opened_view_interface.h"
#include "profile/window_definition.h"

#include <memory>
#include <string>

class CommandHandler;
class ContentsModel;
class Controller;
class DialogService;
class MainWindow;
class WindowDefinition;
class PrintService;
struct WindowInfo;

using PopupMenuHandler = std::function<void(aui::MenuModel* merge_menu,
                                            unsigned resource_id,
                                            const aui::Point& point,
                                            bool right_click)>;

using DefaultNodeCommandHandler = std::function<void(const NodeRef& node)>;

struct OpenedViewContext {
  const std::shared_ptr<Executor> executor_;
  MainWindow* main_window_ = nullptr;
  const WindowInfo& window_info_;
  WindowDefinition& window_def_;
  DialogService& dialog_service_;
  const ControllerFactory controller_factory_;
  const PopupMenuHandler popup_menu_handler_;
  const DefaultNodeCommandHandler default_node_command_handler_;
};

class OpenedView final : private OpenedViewContext,
                         private ControllerDelegate,
                         public OpenedViewInterface {
 public:
  explicit OpenedView(OpenedViewContext&& context);
  virtual ~OpenedView();

  void Init();

  Controller& controller() { return *controller_; }
  const WindowInfo& window_info() const { return window_info_; }
  const WindowInfo& GetWindowInfo() const override { return window_info_; }
  WindowDefinition& window_def() { return window_def_; }
  int window_id() const { return window_def_.id; }
  MainWindow& main_window() const {
    assert(main_window_);
    return *main_window_;
  }
  bool locked() const { return locked_; }

  UiView* view() const { return view_.get(); }
  UiView* ReleaseView() { return view_.release(); }

  void Print(PrintService& print_service);

  void Activate();
  void Select(const scada::NodeId& node_id) override;
  ContentsModel* GetContents() override;
  void SetWindowTitle(std::u16string_view title) override;
  WindowDefinition Save() override;
  std::u16string GetWindowTitle() const override;
  virtual void Close() override;

  // ControllerDelegate
  // TODO: Move to private.
  virtual void SetModified(bool modified) override;

  promise<WindowDefinition> GetOpenWindowDefinition(
      const WindowInfo* window_info) const override;

  std::unique_ptr<CommandHandler> commands;

 private:
  void UpdateWorking();
  void UpdateTitle();

  // ControllerDelegate
  virtual void SetTitle(std::u16string_view) override;
  virtual void ShowPopupMenu(aui::MenuModel* merge_menu,
                             unsigned resource_id,
                             const aui::Point& point,
                             bool right_click) override;
  virtual void OpenView(const WindowDefinition& def) override;
  virtual void ExecuteDefaultNodeCommand(const NodeRef& node) override;
  virtual ContentsModel* GetActiveContentsModel() override;
  virtual void AddContentsObserver(ContentsObserver& observer) override;
  virtual void RemoveContentsObserver(ContentsObserver& observer) override;
  virtual void Focus() override;

  std::unique_ptr<Controller> controller_;

  bool modified_ = false;

  std::u16string title_;

  bool working_ = false;
  ExecutorTimer update_working_timer_;

  std::unique_ptr<UiView> view_;

  // TODO: Next members should be out of this class.

  std::u16string user_title_;
  // Window is locked for adding of new items.
  bool locked_ = false;
};
