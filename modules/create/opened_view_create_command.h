#pragma once

#include "base/any_executor.h"
#include "base/awaitable.h"
#include "base/cancelation.h"
#include "common/node_state.h"
#include "controller/command_handler.h"
#include "node_service/node_ref.h"
#include "scada/session_service.h"
#include "scada/status.h"

#include <functional>

class Controller;
class CreateTree;
class DialogService;
class LocalEvents;
class NodeService;
class Profile;
class TaskManager;

using CreatedNodeHandler = std::function<Awaitable<void>(NodeRef)>;

struct OpenedViewCreateCommandContext {
  AnyExecutor executor_;
  DialogService& dialog_service_;
  scada::SessionService& session_service_;
  NodeService& node_service_;
  TaskManager& task_manager_;
  LocalEvents& local_events_;
  Profile& profile_;
  CreateTree& create_tree_;
  Controller& controller_;
  CreatedNodeHandler created_node_handler_;
};

class OpenedViewCreateCommand final : private OpenedViewCreateCommandContext,
                                      public CommandHandler {
 public:
  explicit OpenedViewCreateCommand(OpenedViewCreateCommandContext&& context);
  ~OpenedViewCreateCommand();

  // CommandHandler
  virtual CommandHandler* GetCommandHandler(unsigned command_id) override;
  virtual void ExecuteCommand(unsigned command_id) override;

 private:
  bool CanCreateRecord(const scada::NodeId& type_node_id) const;
  void CreateRecord(const scada::NodeId& type_node_id, int tag);
  Awaitable<void> CreateRecordAsync(scada::NodeId type_node_id,
                                    scada::NodeId parent_id,
                                    std::u16string title,
                                    scada::NodeAttributes attributes,
                                    scada::NodeProperties properties);
  Awaitable<void> OnCreateRecordCompleteAsync(scada::NodeId node_id);

  Cancelation cancelation_;
};
