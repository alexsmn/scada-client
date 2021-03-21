#pragma once

#include "controls/types.h"
#include "node_service/node_ref.h"
#include "time_range.h"

#include <cassert>
#include <optional>

class CommandHandler;
class ContentsModel;
class ExportModel;
class SelectionModel;
class TimeModel;
class WindowDefinition;

struct OpenContext {
  NodeRef node;
  std::vector<scada::NodeId> node_ids;
  std::optional<TimeRange> time_range;
};

class Controller {
 public:
  virtual ~Controller() = default;

  virtual SelectionModel* GetSelectionModel() { return nullptr; }

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
};
