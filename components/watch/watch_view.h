#pragma once

#include "command_registry.h"
#include "controller.h"
#include "controller_context.h"
#include "export_model.h"
#include "ui/base/models/table_model_observer.h"

#include <memory>

class Table;
class WatchModel;

class WatchView : protected ControllerContext,
                  public Controller,
                  private ui::TableModelObserver,
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
  std::wstring MakeTitle() const;

  void SaveLog();

  // ui::TableModelObserver
  virtual void OnItemsAdded(int first, int count) override;

  std::unique_ptr<WatchModel> model_;

  bool auto_scroll_ = false;

  std::unique_ptr<Table> table_;

  CommandRegistry command_registry_;
};
