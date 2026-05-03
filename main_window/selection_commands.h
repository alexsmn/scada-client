#pragma once

#include "base/any_executor.h"

#include "base/cancelation.h"
#include "base/awaitable.h"
#include "controller/action_manager.h"
#include "profile/window_definition.h"

#include <vector>
#include <unordered_set>

namespace scada {
class SessionService;
}  // namespace scada

class Controller;
class DialogService;
class FileCache;
class MainWindowInterface;
class MainWindowManager;
class NodeEventProvider;
class NodeRef;
class NodeService;
class OpenedViewInterface;
class Profile;
class SelectionModel;
class TaskManager;
struct SelectionCommandContext;

struct SelectionCommandsContext {
  const AnyExecutor executor_;
  TaskManager& task_manager_;
  scada::SessionService& session_service_;
  NodeEventProvider& node_event_provider_;
  FileCache& file_cache_;
  Profile& profile_;
  MainWindowManager& main_window_manager_;
  NodeService& node_service_;
  ActionManager& action_manager_;
};

// A singleton shared between |OpenedView|s. Once an |OpenView| is focused, it
// calls |SetContext|.
class SelectionCommands : private SelectionCommandsContext {
 public:
  explicit SelectionCommands(SelectionCommandsContext&& context);

  SelectionModel* selection() { return selection_; }
  DialogService* dialog_service() { return dialog_service_; }
  MainWindowInterface* main_window() { return main_window_; }

  void SetContext(MainWindowInterface* main_window,
                  DialogService* dialog_service,
                  OpenedViewInterface* opened_view,
                  Controller* controller,
                  SelectionModel* selection);

  void OpenWindow(const WindowInfo* window_info);
  void OpenWindow(const WindowDefinition& window_definition);
  bool IsSelectionAction(unsigned command_id) const;

 private:
  Action& AddAction(Action action);

  void DeleteSelection(const SelectionCommandContext& context);
  void CopyToClipboard(const SelectionCommandContext& context);

  Awaitable<OpenedViewInterface*> OpenViewContainingNode(
      int view_type_id,
      const NodeRef& node,
      MainWindowInterface& main_window,
      DialogService& dialog_service);
  Awaitable<OpenedViewInterface*> OpenViewContainingNodeAsync(
      int view_type_id,
      NodeRef node,
      MainWindowInterface& main_window,
      DialogService& dialog_service);

  SelectionModel* selection_ = nullptr;
  MainWindowInterface* main_window_ = nullptr;
  OpenedViewInterface* opened_view_ = nullptr;
  DialogService* dialog_service_ = nullptr;
  Controller* controller_ = nullptr;
  std::unordered_set<unsigned> selection_action_ids_;

  Cancelation cancelation_;
};
