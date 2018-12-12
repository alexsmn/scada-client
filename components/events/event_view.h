#pragma once

#include "contents_model.h"
#include "controller.h"
#include "time_model.h"

#include <memory>

class Table;
class EventTableModel;

class EventView : public Controller, public ContentsModel, public TimeModel {
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
  virtual bool IsCommandChecked(unsigned command) const override;
  virtual bool IsCommandEnabled(unsigned command) const override;
  virtual void ExecuteCommand(unsigned command) override;
  virtual ContentsModel* GetContentsModel() override { return this; }
  virtual TimeModel* GetTimeModel() override;
  virtual void Print(PrintService& print_service) override;

  // ContentsModel
  virtual void AddContainedItem(const scada::NodeId& node_id,
                                unsigned flags) override;
  virtual void RemoveContainedItem(const scada::NodeId& node_id) override;
  virtual NodeIdSet GetContainedItems() const override;

  // TimeModel
  virtual TimeRange GetTimeRange() const override;
  virtual void SetTimeRange(const TimeRange& time_range) override;

 private:
  base::string16 MakeTitle() const;

  void ExportToExcel();
  void SelectSeverity();

  NodeIdSet EventView::GetSelectedNodeIds() const;

  void OnSelectionChanged();

  bool OnKeyPressed(KeyCode key_code);

  bool is_panel_;

  std::unique_ptr<EventTableModel> model_;
  std::unique_ptr<Table> table_;
};
