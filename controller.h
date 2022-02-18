#pragma once

#include "controls/types.h"
#include "open_context.h"

#include <cassert>
#include <optional>

#if defined(UI_VIEWS)
namespace views {
class DropController;
}
#endif

class CommandHandler;
class ContentsModel;
class ExportModel;
class SelectionModel;
class TimeModel;
class WindowDefinition;

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
