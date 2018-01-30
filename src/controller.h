#pragma once

#include <cassert>

#include "command_handler.h"
#include "controller_delegate.h"
#include "controls/types.h"
#include "core/configuration_types.h"
#include "dialog_service.h"
#include "selection_model.h"

namespace events {
class EventManager;
}

namespace scada {
class MonitoredItemService;
class NodeManagementService;
class HistoryService;
class SessionService;
class ViewService;
}  // namespace scada

#if defined(UI_VIEWS)
namespace views {
class DropController;
}
#endif

class ContentsModel;
class ControllerDelegate;
class Favourites;
class FileCache;
class LocalEvents;
class NodeService;
class PortfolioManager;
class Profile;
class TaskManager;
class TimedDataService;
class WindowDefinition;

struct ControllerContext {
  ControllerDelegate& controller_delegate_;
  TimedDataService& timed_data_service_;
  NodeService& node_service_;
  PortfolioManager& portfolio_manager_;
  TaskManager& task_manager_;
  Profile& profile_;
  LocalEvents& local_events_;
  events::EventManager& event_manager_;
  FileCache& file_cache_;
  scada::NodeManagementService& node_management_service_;
  scada::HistoryService& history_service_;
  Favourites& favourites_;
  DialogService& dialog_service_;
  scada::SessionService& session_service_;
  scada::MonitoredItemService& monitored_item_service_;
  scada::ViewService& view_service_;
};

class Controller : public CommandHandler, protected ControllerContext {
 public:
  explicit Controller(const ControllerContext& context)
      : ControllerContext(std::move(context)),
        selection_{node_service_, timed_data_service_} {}
  virtual ~Controller() {}

  SelectionModel& selection() { return selection_; }

  virtual UiView* Init(const WindowDefinition& definition) = 0;

  virtual bool CanClose() const { return true; }
  virtual bool IsWorking() const { return false; }

  virtual void Save(WindowDefinition& definition) {}
  virtual void OnViewNodeCreated(const NodeRef& node) {}

  virtual bool ShowContainedItem(const scada::NodeId& item_id) { return false; }

  virtual ContentsModel* GetContentsModel() { return nullptr; }

#if defined(UI_VIEWS)
  virtual views::DropController* GetDropController() { return nullptr; }
#endif

  // View root node for creation.
  virtual NodeRef GetRootNode() const { return nullptr; }

 private:
  SelectionModel selection_;
};
