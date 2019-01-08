#pragma once

#include "contents_model.h"
#include "controller.h"
#include "export_model.h"

class Table;
class TableModel;

class TableView : public Controller, public ContentsModel, public ExportModel {
 public:
  explicit TableView(const ControllerContext& context);
  virtual ~TableView();

  void DeleteSelection();

  // Controller
  virtual UiView* Init(const WindowDefinition& definition) override;
  virtual void Save(WindowDefinition& definition) override;
  virtual ContentsModel* GetContentsModel() override { return this; }
  virtual ExportModel* GetExportModel() override { return this; }

  // ContentsModel
  virtual void AddContainedItem(const scada::NodeId& node_id,
                                unsigned flags) override;
  virtual void RemoveContainedItem(const scada::NodeId& node_id) override;
  virtual NodeIdSet GetContainedItems() const override;

  // CommandHandler
  virtual CommandHandler* GetCommandHandler(unsigned command_id) override;
  virtual void ExecuteCommand(unsigned command) override;

  // ExportModel
  virtual ExportData GetExportData() override;

 private:
  void MoveRow(bool up);

  NodeIdSet GetMultipleSelection();

  void OnSelectionChanged();
  void OnDoubleClick();
  bool OnKeyPressed(KeyCode key_code);

  std::unique_ptr<TableModel> model_;
  std::unique_ptr<Table> view_;
};
