#pragma once

#include "aui/types.h"
#include "base/awaitable.h"
#include "controller/open_context.h"

#include <cassert>
#include <optional>

class CommandHandler;
class ContentsModel;
class ExportModel;
class ImageExportModel;
class SelectionModel;
class SeriesModel;
class TimeModel;
class WindowDefinition;

class Controller {
 public:
  virtual ~Controller() = default;

  virtual SelectionModel* GetSelectionModel() { return nullptr; }

  virtual std::unique_ptr<UiView> Init(const WindowDefinition& definition) = 0;

  virtual void Save(WindowDefinition& definition) {}

  // True while the view holds changes the operator made but has not written
  // (e.g. staged property edits). An operator-requested close asks
  // ResolvePendingChanges first only when this is true, so a view with nothing
  // pending still closes synchronously.
  virtual bool HasPendingChanges() const { return false; }

  // Settles the pending changes before an operator-requested close — typically
  // by asking whether to apply or discard them. Resolves true when the view may
  // close, false when the operator chose to keep it open. The controller may be
  // destroyed while this is suspended (the view can close for other reasons
  // while a prompt is up), so callers re-find the view after the await.
  virtual Awaitable<bool> ResolvePendingChanges() { co_return true; }
  virtual void OnViewNodeCreated(const NodeRef& node) {}

  virtual bool ShowContainedItem(const scada::NodeId& node_id) { return false; }

  virtual CommandHandler* GetCommandHandler(unsigned command_id) {
    return nullptr;
  }

  virtual ContentsModel* GetContentsModel() { return nullptr; }

  virtual TimeModel* GetTimeModel() { return nullptr; }

  // The plotted series the view currently has configurable, for the shell
  // Inspector's series section. Null for a view that plots nothing.
  virtual SeriesModel* GetSeriesModel() { return nullptr; }

  virtual ExportModel* GetExportModel() { return nullptr; }

  // A view whose contents are a picture to print or save as an image.
  virtual ImageExportModel* GetImageExportModel() { return nullptr; }

  // View root node for creation.
  virtual NodeRef GetRootNode() const { return nullptr; }

  virtual std::optional<OpenContext> GetOpenContext() const {
    return std::nullopt;
  }
};
