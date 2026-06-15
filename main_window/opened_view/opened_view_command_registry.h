#pragma once

#include "base/any_executor.h"
#include "base/awaitable.h"

#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace scada {
class SessionService;
}

class CommandHandler;
class Controller;
class CreateTree;
class DialogService;
class ExportModel;
class LocalEvents;
class NodeRef;
class NodeService;
class PrintService;
class Profile;
class TaskManager;
class TimeModel;

using OpenedViewCreatedNodeHandler = std::function<Awaitable<void>(NodeRef)>;
using OpenedViewExportModelGetter = std::function<ExportModel*()>;
using OpenedViewPrintViewHandler = std::function<void(PrintService&)>;
using OpenedViewTimeModelGetter = std::function<TimeModel*()>;
using OpenedViewWindowTitleGetter = std::function<std::u16string()>;

struct OpenedViewCommandFactoryContext {
  AnyExecutor executor_;
  DialogService& dialog_service_;
  scada::SessionService& session_service_;
  NodeService& node_service_;
  TaskManager& task_manager_;
  LocalEvents& local_events_;
  Profile& profile_;
  CreateTree& create_tree_;
  Controller& controller_;
  PrintService* print_service_ = nullptr;
  OpenedViewExportModelGetter export_model_getter_;
  OpenedViewTimeModelGetter time_model_getter_;
  OpenedViewWindowTitleGetter window_title_getter_;
  OpenedViewPrintViewHandler print_view_handler_;
  OpenedViewCreatedNodeHandler created_node_handler_;
};

using OpenedViewCommandFactory =
    std::function<std::unique_ptr<CommandHandler>(
        const OpenedViewCommandFactoryContext&)>;

class OpenedViewCommandRegistry {
 public:
  void AddFactory(OpenedViewCommandFactory factory) {
    factories_.emplace_back(std::move(factory));
  }

  std::vector<std::unique_ptr<CommandHandler>> CreateCommandHandlers(
      const OpenedViewCommandFactoryContext& context) const {
    std::vector<std::unique_ptr<CommandHandler>> handlers;
    handlers.reserve(factories_.size());
    for (const auto& factory : factories_) {
      if (auto handler = factory(context)) {
        handlers.emplace_back(std::move(handler));
      }
    }
    return handlers;
  }

 private:
  std::vector<OpenedViewCommandFactory> factories_;
};
