#pragma once

#include "aui/models/table_model_observer.h"
#include "base/promise.h"
#include "controller/command_registry.h"
#include "controller/controller.h"
#include "controller/controller_context.h"
#include "controller/time_model.h"
#include "export/export_model.h"

#include <memory>

namespace aui {
class Table;
}

class WatchModel;

class WatchView : protected ControllerContext,
                  public Controller,
                  private aui::TableModelObserver,
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

  promise<> SaveLog();

  // aui::TableModelObserver
  virtual void OnItemsAdded(int first, int count) override;

  const std::shared_ptr<WatchModel> model_;

  bool auto_scroll_ = false;

  aui::Table* table_ = nullptr;

  CommandRegistry command_registry_;
};
