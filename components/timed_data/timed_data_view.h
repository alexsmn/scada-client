#pragma once

#include "command_handler.h"
#include "components/timed_data/timed_data_model.h"
#include "contents_model.h"
#include "controller.h"
#include "export_model.h"
#include "ui/base/models/mirror_table_model.h"

#include <memory>

class Table;
class TimedDataModel;

class TimedDataView : public Controller,
                      public CommandHandler,
                      public ContentsModel,
                      public ExportModel {
 public:
  explicit TimedDataView(const ControllerContext& context);

  // Controller
  virtual UiView* Init(const WindowDefinition& definition) override;
  virtual void Save(WindowDefinition& definition) override;
  virtual bool IsWorking() const override;
  virtual CommandHandler* GetCommandHandler(unsigned command_id) override;
  virtual void ExecuteCommand(unsigned command) override;
  virtual ContentsModel* GetContentsModel() override { return this; }
  virtual TimeModel* GetTimeModel() override;
  virtual ExportModel* GetExportModel() override { return this; }
  virtual std::optional<OpenContext> GetOpenContext() const override;

  // ContentsModel
  virtual void AddContainedItem(const scada::NodeId& node_id,
                                unsigned flags) override;

  // ExportModel
  virtual ExportData GetExportData() override;

 private:
  std::wstring MakeTitle() const;
  void UpdateColumnTitles();

  TimedDataModel model_{TimedDataModelContext{timed_data_service_}};
  ui::MirrorTableModel mirror_model_{model_};

  std::unique_ptr<Table> view_;
};
