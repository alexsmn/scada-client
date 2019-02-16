#pragma once

#include "command_handler_impl.h"
#include "common_resources.h"
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
  virtual CommandHandler* GetCommandHandler(unsigned command_id) override;

  // ContentsModel
  virtual void AddContainedItem(const scada::NodeId& node_id,
                                unsigned flags) override;
  virtual void RemoveContainedItem(const scada::NodeId& node_id) override;
  virtual NodeIdSet GetContainedItems() const override;

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

  CommandHandlerImpl command_handler_;
  Command& delete_command_ = command_handler_.AddCommand(ID_DELETE);
  Command& rename_command_ = command_handler_.AddCommand(ID_RENAME);
  Command& move_up_command_ = command_handler_.AddCommand(ID_MOVE_UP);
  Command& move_down_command_ = command_handler_.AddCommand(ID_MOVE_DOWN);
  Command& sort_name_command_ = command_handler_.AddCommand(ID_SORT_NAME);
  Command& sort_channel_command_ = command_handler_.AddCommand(ID_SORT_CHANNEL);
};
