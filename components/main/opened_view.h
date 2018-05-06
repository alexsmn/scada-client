#pragma once

#include "base/strings/string16.h"
#include "base/timer/timer.h"
#include "controller_delegate.h"
#include "controller_factory.h"
#include "controls/types.h"
#include "window_definition.h"

#if defined(UI_VIEWS)
#include "ui/gfx/image/image.h"
#include "ui/views/context_menu_controller.h"
#include "ui/views/drop_controller.h"
#endif

#include <memory>

class CommandHandler;
class ContentsModel;
class Controller;
class DialogService;
class MainWindow;
class WindowDefinition;
struct WindowInfo;

namespace ui {
class MenuModel;
}

struct OpenedViewContext {
  MainWindow* main_window_;
  WindowDefinition& window_def_;
  DialogService& dialog_service_;
  ControllerFactory controller_factory_;
  ui::MenuModel& context_menu_model_;
};

class OpenedView : private OpenedViewContext,
#if defined(UI_VIEWS)
                   public views::DropController,
                   private views::ContextMenuController,
#endif
                   private ControllerDelegate {
 public:
  explicit OpenedView(OpenedViewContext&& context);
  virtual ~OpenedView();

  Controller& controller() { return *controller_; }
  const WindowInfo& window_info() const { return window_def_.window_info(); }
  WindowDefinition& window_def() { return window_def_; }
  int window_id() const { return window_def_.id; }
  MainWindow& main_window() const {
    assert(main_window_);
    return *main_window_;
  }
  bool locked() const { return locked_; }

  UiView* view() { return view_; }

#if defined(UI_VIEWS)
  const gfx::Image& image() const { return image_; }
#endif

  void Print();

  void Activate();
  void SetSelection(const scada::NodeId& item_id);
  ContentsModel* GetContentsModel();
  void SetUserTitle(const base::StringPiece16& title);
  void Save();
  base::string16 GetWindowTitle() const;
  void Close();

#if defined(UI_VIEWS)
  // views::DropController
  virtual bool CanDrop(const ui::OSExchangeData& data) override;
  virtual void OnDragEntered(const ui::DropTargetEvent& event) override;
  virtual int OnDragUpdated(const ui::DropTargetEvent& event) override;
  virtual void OnDragDone() override;
  virtual int OnPerformDrop(const ui::DropTargetEvent& event) override;

  // views::ContextMenuController
  virtual void ShowContextMenuForView(views::View* source,
                                      const gfx::Point& point) override;
#endif

  // ControllerDelegate
  // TODO: Move to private.
  virtual void SetModified(bool modified) override;

  std::unique_ptr<CommandHandler> commands;

 private:
  void UpdateWorking();
  void UpdateTitle();

  // ControllerDelegate
  virtual void SetTitle(const base::StringPiece16& title) override;
  virtual void ShowPopupMenu(unsigned resource_id,
                             const gfx::Point& point,
                             bool right_click) override;
  virtual void OpenView(const WindowDefinition& def) override;
  virtual void ExecuteDefaultNodeCommand(const NodeRef& node) override;
  virtual ContentsModel* GetActiveContentsModel() override;
  virtual void AddContentsObserver(ContentsObserver& observer) override;
  virtual void RemoveContentsObserver(ContentsObserver& observer) override;

  std::unique_ptr<Controller> controller_;

  bool modified_ = false;

  base::string16 title_;

  bool working_ = false;
  base::RepeatingTimer update_working_timer_;

  UiView* view_ = nullptr;

#if defined(UI_VIEWS)
  gfx::Image image_;
#endif

  // TODO: Next members should be out of this class.

  base::string16 user_title_;
  // Window is locked for adding of new items.
  bool locked_ = false;
};
