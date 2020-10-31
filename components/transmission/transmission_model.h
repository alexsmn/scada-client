#pragma once

#include <functional>
#include <vector>

#include "node_service/node_observer.h"
#include "node_service/node_ref.h"
#include "contents_model.h"
#include "ui/base/models/fixed_row_model.h"
#include "ui/base/models/grid_model.h"

class NodeService;
class TaskManager;

class TransmissionModel : public ui::GridModel,
                          public views::FixedRowModel,
                          public ContentsModel,
                          private views::FixedRowModel::Delegate,
                          private NodeRefObserver {
 public:
  struct Row {
    NodeRef transmission;
    scada::NodeId source_id;
  };

  TransmissionModel(NodeService& node_service, TaskManager& task_manager);
  ~TransmissionModel();

  NodeRef device() const { return device_; }
  void SetDevice(NodeRef device);

  const Row& row(size_t index) const { return rows_[index]; }

  typedef std::vector<Row> Rows;
  const Rows& rows() const { return rows_; }

  void Update();

  // ContentsModel
  virtual void AddContainedItem(const scada::NodeId& node_id,
                                unsigned flags) override;
  virtual void RemoveContainedItem(const scada::NodeId& node_id) override;
  virtual NodeIdSet GetContainedItems() const override;

  // GridModel
  virtual int GetRowCount() override;
  virtual base::string16 GetRowTitle(int row) override;
  virtual void GetCell(ui::GridCell& cell) override;
  virtual bool IsEditable(int row, int column) override;
  virtual bool SetCellText(int row, int column, const base::string16& text) override;

 private:
  int FindRow(const scada::NodeId& transmission_id) const;
  int FindSource(const scada::NodeId& source_id) const;

  void Update(NodeRef transmission);
  void Delete(const scada::NodeId& transmission_id);

  static Row MakeRow(NodeRef transmission);

  // NodeRefObserver
  virtual void OnModelChanged(const scada::ModelChangeEvent& event) override;
  virtual void OnNodeSemanticChanged(const scada::NodeId& node_id) override;

  NodeService& node_service_;
  TaskManager& task_manager_;

  Rows rows_;
  NodeRef device_;

  DISALLOW_COPY_AND_ASSIGN(TransmissionModel);
};
