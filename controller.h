#pragma once

#include <cassert>
#include <optional>

#include "common/aliases.h"
#include "controller_delegate.h"
#include "controls/types.h"
#include "core/configuration_types.h"
#include "selection_model.h"
#include "time_range.h"

namespace scada {
class MonitoredItemService;
class HistoryService;
class SessionService;
}  // namespace scada

class CommandHandler;
class ContentsModel;
class ControllerDelegate;
class DialogService;
class EventFetcher;
class ExportModel;
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
  EventFetcher& event_fetcher_;
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

struct OpenContext {
  std::vector<scada::NodeId> node_ids;
  base::string16 title;
  std::optional<TimeRange> time_range;
};

class Controller : protected ControllerContext {
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

  virtual CommandHandler* GetCommandHandler(unsigned command_id) {
    return nullptr;
  }

  virtual ContentsModel* GetContentsModel() { return nullptr; }

  virtual TimeModel* GetTimeModel() { return nullptr; }

  virtual ExportModel* GetExportModel() { return nullptr; }

#if defined(UI_VIEWS)
  virtual views::DropController* GetDropController() { return nullptr; }
#endif

  // View root node for creation.
  virtual NodeRef GetRootNode() const { return nullptr; }

  virtual std::optional<OpenContext> GetOpenContext() const {
    return std::nullopt;
  }

 private:
  SelectionModel selection_;
};
