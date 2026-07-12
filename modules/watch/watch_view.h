#pragma once

#include "aui/aui_ns_compat.h"

#include "controller/command_registry.h"
#include "controller/controller.h"
#include "controller/controller_context.h"
#include "controller/time_model.h"
#include "export/export_model.h"
#include "modules/watch/watch_menu_model.h"

#include <boost/signals2/connection.hpp>
#include <memory>

namespace scada::aui {
class Table;
}

class WatchModel;

class WatchView : protected ControllerContext,
                  public Controller,
                  public TimeModel,
                  public ExportModel {
 public:
  explicit WatchView(const ControllerContext& context);
  virtual ~WatchView();

  // Controller
  virtual std::unique_ptr<UiView> Init(
      const WindowDefinition& definition) override;
  virtual void Save(WindowDefinition& definition) override;
  virtual CommandHandler* GetCommandHandler(unsigned command_id) override;
  virtual TimeModel* GetTimeModel() override { return this; }
  virtual ExportModel* GetExportModel() override { return this; }

  // TimeModel
  virtual TimeRange GetTimeRange() const override;
  virtual void SetTimeRange(const TimeRange& time_range) override;

  // ExportModel
  virtual ExportData GetExportData() override;

 private:
  std::u16string MakeTitle() const;

  void SaveLog();

  void OnItemsAdded(int first, int count);

  const std::shared_ptr<WatchModel> model_;

  bool auto_scroll_ = false;

  aui::Table* table_ = nullptr;

  CommandRegistry command_registry_;

  // Cross-platform context menu, backed by `command_registry_`. Declared after
  // it so the registry outlives the menu's delegate.
  WatchMenuModel watch_menu_model_{command_registry_};

  boost::signals2::scoped_connection items_added_connection_;
};
