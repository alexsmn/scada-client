#pragma once

#include "aui/models/table_model_observer.h"
#include "base/promise.h"
#include "command_registry.h"
#include "controller/controller.h"
#include "controller/controller_context.h"
#include "export_model.h"

#include <memory>

namespace aui {
class Table;
}

class WatchModel;

class WatchView : protected ControllerContext,
                  public Controller,
                  private aui::TableModelObserver,
                  public ExportModel {
 public:
  explicit WatchView(const ControllerContext& context);
  virtual ~WatchView();

  // Controller
  virtual UiView* Init(const WindowDefinition& definition) override;
  virtual void Save(WindowDefinition& definition) override;
  virtual CommandHandler* GetCommandHandler(unsigned command_id) override;
  virtual ExportModel* GetExportModel() override { return this; }

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
