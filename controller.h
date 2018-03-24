#pragma once

#include <cassert>

#include "command_handler.h"
#include "common/aliases.h"
#include "controller_delegate.h"
#include "controls/types.h"
#include "core/configuration_types.h"
#include "selection_model.h"

namespace events {
class EventManager;
}

namespace scada {
class MonitoredItemService;
class HistoryService;
class SessionService;
}  // namespace scada

class ContentsModel;
class ControllerDelegate;
class DialogService;
class Favourites;
class FileCache;
class LocalEvents;
class NodeService;
class PortfolioManager;
class Profile;
class TaskManager;
class TimedDataService;
class TimeModel;
class WindowDefinition;

struct ControllerContext {
  ControllerDelegate& controller_delegate_;
  const AliasResolver alias_resolver_;
  TaskManager& task_manager_;
  scada::SessionService& session_service_;
  events::EventManager& event_manager_;
  scada::HistoryService& history_service_;
  scada::MonitoredItemService& monitored_item_service_;
  TimedDataService& timed_data_service_;
  NodeService& node_service_;
  PortfolioManager& portfolio_manager_;
  LocalEvents& local_events_;
  Favourites& favourites_;
  FileCache& file_cache_;
  Profile& profile_;
  DialogService& dialog_service_;
};

class Controller : protected ControllerContext, public CommandHandler {
 public:
  explicit Controller(const ControllerContext& context)
      : ControllerContext{context},
        selection_{SelectionModelContext{timed_data_service_}} {}
  virtual ~Controller() {}

  SelectionModel& selection() { return selection_; }

  virtual UiView* Init(const WindowDefinition& definition) = 0;

  virtual bool CanClose() const { return true; }
  virtual bool IsWorking() const { return false; }

  virtual void Save(WindowDefinition& definition) {}
  virtual void OnViewNodeCreated(const NodeRef& node) {}

  virtual bool ShowContainedItem(const scada::NodeId& item_id) { return false; }

  virtual ContentsModel* GetContentsModel() { return nullptr; }

  virtual TimeModel* GetTimeModel() { return nullptr; }

#if defined(UI_VIEWS)
  virtual views::DropController* GetDropController() { return nullptr; }
#endif

  // View root node for creation.
  virtual NodeRef GetRootNode() const { return nullptr; }

 private:
  SelectionModel selection_;
};
