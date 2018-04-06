#pragma once

#include <memory>

#include "base/memory/weak_ptr.h"
#include "base/strings/string16.h"
#include "base/timer/timer.h"
#include "command_handler.h"
#include "common/aliases.h"
#include "controller_delegate.h"
#include "controls/types.h"
#include "core/configuration_types.h"
#include "core/status.h"

#if defined(UI_VIEWS)
#include "ui/gfx/image/image.h"
#include "ui/views/context_menu_controller.h"
#include "ui/views/drop_controller.h"
#endif

namespace events {
class EventManager;
}

namespace scada {
class HistoryService;
class MethodService;
class MonitoredItemService;
class NodeManagementService;
class SessionService;
}  // namespace scada

class ActionManager;
class ContentsModel;
class Controller;
class DialogService;
class Favourites;
class FileCache;
class LocalEvents;
class MainWindow;
class MainWindowManager;
class NodeService;
class PortfolioManager;
class Profile;
class SelectionCommands;
class TaskManager;
class TimedDataService;
class WindowDefinition;
struct WindowInfo;

struct OpenedViewContext {
  MainWindow* main_window_;
  const AliasResolver alias_resolver_;
  TaskManager& task_manager_;
  scada::MethodService& method_service_;
  scada::SessionService& session_service_;
  scada::NodeManagementService& node_management_service_;
  events::EventManager& event_manager_;
  scada::HistoryService& history_service_;
  scada::MonitoredItemService& monitored_item_service_;
  TimedDataService& timed_data_service_;
  NodeService& node_service_;
  PortfolioManager& portfolio_manager_;
  ActionManager& action_manager_;
  LocalEvents& local_events_;
  Favourites& favourites_;
  FileCache& file_cache_;
  Profile& profile_;
  DialogService& dialog_service_;
  MainWindowManager& main_window_manager_;
};

class OpenedView : private OpenedViewContext,
                   public CommandHandler,
#if defined(UI_VIEWS)
                   public views::DropController,
                   private views::ContextMenuController,
#endif
                   private ControllerDelegate {
 public:
  OpenedView(const OpenedViewContext& context,
             const WindowDefinition& definition);
  virtual ~OpenedView();

  Controller& controller() { return *controller_; }
  const WindowInfo& window_info() const { return window_info_; }
  int window_id() const { return window_id_; }
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
  void Save(WindowDefinition& definition);
  base::string16 GetWindowTitle() const;
  void Close();

  // CommandHandler
  virtual CommandHandler* GetCommandHandler(unsigned command_id) override;
  virtual void ExecuteCommand(unsigned command_id) override;
  virtual bool IsCommandChecked(unsigned command_id) const override;

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

 private:
  void UpdateWorking();
  void UpdateTitle();

  bool CanCreateRecord(const scada::NodeId& type_node_id) const;
  void CreateRecord(const scada::NodeId& type_node_id, int tag);
  void OnCreateRecordComplete(const scada::LocalizedText& display_name,
                              const scada::Status& status,
                              const scada::NodeId& node_id);

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

  const WindowInfo& window_info_;
  int window_id_;

  std::unique_ptr<SelectionCommands> selection_commands_;
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

  base::WeakPtrFactory<OpenedView> weak_factory_{this};
};
