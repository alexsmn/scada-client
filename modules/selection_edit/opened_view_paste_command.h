#pragma once

#include "base/any_executor.h"
#include "base/cancelation.h"
#include "controller/command_handler.h"
#include "scada/session_service.h"

class Controller;
class CreateTree;
class NodeService;
class TaskManager;

struct OpenedViewPasteCommandContext {
  AnyExecutor executor_;
  scada::SessionService& session_service_;
  NodeService& node_service_;
  TaskManager& task_manager_;
  CreateTree& create_tree_;
  Controller& controller_;
};

class OpenedViewPasteCommand final : private OpenedViewPasteCommandContext,
                                     public CommandHandler {
 public:
  explicit OpenedViewPasteCommand(OpenedViewPasteCommandContext&& context);
  ~OpenedViewPasteCommand();

  // CommandHandler
  virtual CommandHandler* GetCommandHandler(unsigned command_id) override;
  virtual bool IsCommandEnabled(unsigned command_id) const override;
  virtual void ExecuteCommand(unsigned command_id) override;

 private:
  Awaitable<void> PasteFromClipboardAsync();

  Cancelation cancelation_;
};
