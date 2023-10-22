#pragma once

#include "controller/contents_model.h"
#include "aui/models/fixed_row_model.h"
#include "aui/models/grid_model.h"
#include "node_service/node_observer.h"
#include "node_service/node_ref.h"

#include <functional>
#include <vector>

class NodeService;
class TaskManager;

class TransmissionModel
    : private aui::FixedRowModel::Delegate,
      private NodeRefObserver,
      public aui::GridModel,
      public aui::FixedRowModel,
      public ContentsModel,
      public std::enable_shared_from_this<TransmissionModel> {
 public:
  struct Row {
    NodeRef transmission;
    scada::NodeId source_id;
  };

  TransmissionModel(NodeService& node_service, TaskManager& task_manager);
  ~TransmissionModel();

  TransmissionModel(const TransmissionModel&) = delete;
  TransmissionModel& operator=(const TransmissionModel&) = delete;

  void Init(NodeRef device);

  const NodeRef& device() const { return device_; }

  const Row& row(size_t index) const { return rows_[index]; }

  typedef std::vector<Row> Rows;
  const Rows& rows() const { return rows_; }

  void Refresh();

  // ContentsModel
  virtual void AddContainedItem(const scada::NodeId& node_id,
                                unsigned flags) override;
  virtual void RemoveContainedItem(const scada::NodeId& node_id) override;
  virtual NodeIdSet GetContainedItems() const override;

  // GridModel
  virtual int GetRowCount() override;
  virtual std::u16string GetRowTitle(int row) override;
  virtual void GetCell(aui::GridCell& cell) override;
  virtual bool IsEditable(int row, int column) override;
  virtual bool SetCellText(int row,
                           int column,
                           const std::u16string& text) override;

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

  NodeRef device_;

  Rows rows_;
};
