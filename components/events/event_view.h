#pragma once

#include "aui/key_codes.h"
#include "base/promise.h"
#include "command_registry.h"
#include "contents_model.h"
#include "controller/controller.h"
#include "controller/controller_context.h"
#include "export_model.h"
#include "selection_model.h"
#include "time_model.h"

#include <memory>

namespace aui {
class Table;
}

class EventTableModel;

class EventView : protected ControllerContext,
                  public Controller,
                  public ContentsModel,
                  public TimeModel,
                  public ExportModel {
 public:
  EventView(const ControllerContext& context, bool is_panel);
  virtual ~EventView();

  bool CanAcknowledgeSelection() const;
  void AcknowledgeSelection();

  // Controller
  virtual bool IsWorking() const override;
  virtual UiView* Init(const WindowDefinition& definition) override;
  virtual void Save(WindowDefinition& definition) override;
  virtual CommandHandler* GetCommandHandler(unsigned command_id) override;
  virtual SelectionModel* GetSelectionModel() override { return &selection_; }
  virtual ContentsModel* GetContentsModel() override { return this; }
  virtual TimeModel* GetTimeModel() override;
  virtual ExportModel* GetExportModel() override { return this; }

  // ContentsModel
  virtual void AddContainedItem(const scada::NodeId& node_id,
                                unsigned flags) override;
  virtual void RemoveContainedItem(const scada::NodeId& node_id) override;
  virtual NodeIdSet GetContainedItems() const override;

  // TimeModel
  virtual TimeRange GetTimeRange() const override;
  virtual void SetTimeRange(const TimeRange& time_range) override;

  // ExportModel
  virtual ExportData GetExportData() override;

 private:
  std::u16string MakeTitle() const;

  void ExportToExcel();
  promise<> SelectSeverity();

  NodeIdSet GetSelectedNodeIds() const;

  void OnSelectionChanged();

  bool OnKeyPressed(aui::KeyCode key_code);

  const bool is_panel_;

  const std::shared_ptr<EventTableModel> model_;

  SelectionModel selection_{{timed_data_service_}};

  // Owned by the parent widget.
  aui::Table* table_ = nullptr;

  CommandRegistry command_registry_;
};
