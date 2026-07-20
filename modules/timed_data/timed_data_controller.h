#pragma once

#include "modules/timed_data/timed_data_model.h"
#include "controller/command_registry.h"
#include "controller/contents_model.h"
#include "controller/controller.h"
#include "controller/controller_context.h"
#include "controller/selection_model.h"
#include "export/export_model.h"

#include <memory>

namespace scada::aui {
class MirrorTableModel;
class Table;
}  // namespace aui

class TimedDataModel;

class TimedDataController : protected ControllerContext,
                            public Controller,
                            public ContentsModel,
                            public ExportModel {
 public:
  explicit TimedDataController(const ControllerContext& context);

  // Controller
  virtual std::unique_ptr<UiView> Init(
      const WindowDefinition& definition) override;
  virtual void Save(WindowDefinition& definition) override;
  virtual bool IsWorking() const override;
  virtual CommandHandler* GetCommandHandler(unsigned command_id) override;
  virtual SelectionModel* GetSelectionModel() override { return &selection_; }
  virtual ContentsModel* GetContentsModel() override { return this; }
  virtual TimeModel* GetTimeModel() override;
  virtual ExportModel* GetExportModel() override { return this; }
  virtual std::optional<OpenContext> GetOpenContext() const override;

  // ContentsModel
  virtual void AddContainedItem(const scada::NodeId& node_id,
                                unsigned flags) override;

  // ExportModel
  virtual ExportData GetExportData() override;

  // Number of rows currently materialized in the timed-data table. Used by the
  // E2E harness to assert the historical view populated after a HistoryRead.
  int GetRowCountForTesting() const { return model_->GetRowCount(); }

 private:
  std::u16string MakeTitle() const;
  void UpdateColumnTitles();

  SelectionModel selection_{{timed_data_service_}};

  std::shared_ptr<TimedDataModel> model_;
  std::shared_ptr<scada::aui::MirrorTableModel> mirror_model_;

  scada::aui::Table* view_ = nullptr;

  CommandRegistry command_registry_;
};
