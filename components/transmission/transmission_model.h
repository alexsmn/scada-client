#pragma once

#include <functional>
#include <vector>

#include "base/memory/weak_ptr.h"
#include "common/node_ref.h"
#include "common/node_observer.h"
#include "contents_model.h"
#include "ui/base/models/fixed_row_model.h"
#include "ui/base/models/grid_model.h"

namespace scada {
class NodeManagementService;
class ViewService;
}  // namespace scada

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

  TransmissionModel(scada::ViewService& view_service,
                    NodeService& node_service,
                    TaskManager& task_manager,
                    scada::NodeManagementService& node_management_service);
  ~TransmissionModel();

  const NodeRef& device() const { return device_; }
  void SetDevice(NodeRef device);
  void SetDeviceId(const scada::NodeId& device_id);

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

 private:
  int FindRow(const scada::NodeId& transmission_id) const;
  int FindSource(const scada::NodeId& source_id) const;

  void UpdateItem(const NodeRef& transmission);
  void DeleteRow(const scada::NodeId& transmission_id);

  // NodeRefObserver
  virtual void OnModelChange(const ModelChangeEvent& event) override;
  virtual void OnNodeSemanticChanged(const scada::NodeId& node_id) override;

  scada::ViewService& view_service_;
  NodeService& node_service_;
  TaskManager& task_manager_;
  scada::NodeManagementService& node_management_service_;

  Rows rows_;
  NodeRef device_;

  base::WeakPtrFactory<TransmissionModel> weak_ptr_factory_{this};

  DISALLOW_COPY_AND_ASSIGN(TransmissionModel);
};
