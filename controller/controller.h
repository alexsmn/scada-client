#pragma once

#include "aui/types.h"
#include "controller/open_context.h"

#include <cassert>
#include <optional>

class ActionManager;
class ContentsModel;
class ExportModel;
class SelectionModel;
class TimeModel;
class WindowDefinition;

class Controller {
 public:
  virtual ~Controller() = default;

  virtual SelectionModel* GetSelectionModel() { return nullptr; }

  virtual std::unique_ptr<UiView> Init(const WindowDefinition& definition) = 0;

  virtual bool IsWorking() const { return false; }

  virtual void Save(WindowDefinition& definition) {}
  virtual void OnViewNodeCreated(const NodeRef& node) {}

  virtual bool ShowContainedItem(const scada::NodeId& node_id) { return false; }

  virtual ActionManager* GetActionManager() { return nullptr; }

  virtual ContentsModel* GetContentsModel() { return nullptr; }

  virtual TimeModel* GetTimeModel() { return nullptr; }

  virtual ExportModel* GetExportModel() { return nullptr; }

  // View root node for creation.
  virtual NodeRef GetRootNode() const { return nullptr; }

  virtual std::optional<OpenContext> GetOpenContext() const {
    return std::nullopt;
  }
};
