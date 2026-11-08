#pragma once

#include "aui/models/fixed_row_model.h"
#include "aui/models/grid_model.h"
#include "base/any_executor.h"
#include "base/cancelation.h"
#include "base/lifetime.h"
#include "controller/contents_model.h"
#include "modules/transmission/transmission_address.h"
#include "node_service/node_ref.h"

#include <boost/signals2/connection.hpp>
#include <functional>
#include <set>
#include <vector>

class NodeService;
class TaskManager;

class TransmissionModel
    : private scada::aui::FixedRowModel::Delegate,
      public scada::aui::GridModel,
      public scada::aui::FixedRowModel,
      public ContentsModel,
      public std::enable_shared_from_this<TransmissionModel> {
 public:
  struct Row {
    NodeRef transmission;
    scada::NodeId source_id;
  };

  TransmissionModel(AnyExecutor executor,
                    NodeService& node_service,
                    TaskManager& task_manager);
  ~TransmissionModel();

  TransmissionModel(const TransmissionModel&) = delete;
  TransmissionModel& operator=(const TransmissionModel&) = delete;

  void Init(NodeRef device);

  const NodeRef& device() const SCADA_LIFETIME_BOUND { return device_; }

  const Row& row(size_t index) const SCADA_LIFETIME_BOUND {
    return rows_[index];
  }

  typedef std::vector<Row> Rows;
  const Rows& rows() const SCADA_LIFETIME_BOUND { return rows_; }

  void Refresh();

  // True when another rule on this device occupies any of the destination
  // addresses `row`'s rule does: the same address, or for MODBUS a register
  // inside a wider rule's span (a FLOAT at 40001 also holds 40002). Such rows
  // read "Duplicate address" in the Status column; a rule whose address has
  // not been fetched is never one.
  bool IsDuplicateAddress(int row) const;

  // ContentsModel
  virtual void AddContainedItem(const scada::NodeId& node_id,
                                unsigned flags) override;
  virtual void RemoveContainedItem(const scada::NodeId& node_id) override;
  virtual NodeIdSet GetContainedItems() const override;

  // GridModel
  virtual int GetRowCount() override;
  virtual std::u16string GetRowTitle(int row) override;
  virtual void GetCell(scada::aui::GridCell& cell) override;
  virtual bool IsEditable(int row, int column) override;
  virtual bool SetCellText(int row,
                           int column,
                           const std::u16string& text) override;

 private:
  int FindRow(const scada::NodeId& transmission_id) const;
  int FindSource(const scada::NodeId& source_id) const;

  // The fetched destination addresses of every row.
  std::vector<TransmissionAddressSpan> UsedAddresses() const;

  // Forgets the reservation `AddContainedItem` made for `transmission`'s
  // address, now that the rule carrying it has arrived.
  void ReleaseReservedAddress(const NodeRef& transmission);

  void Update(NodeRef transmission);
  void Delete(const scada::NodeId& transmission_id);

  void OnModelChanged(const scada::ModelChangeEvent& event);
  void OnNodeSemanticChanged(const scada::NodeId& node_id);
  void OnNodeFetched(const NodeFetchedEvent& event);

  AnyExecutor executor_;
  NodeService& node_service_;
  TaskManager& task_manager_;

  NodeRef device_;

  Rows rows_;

  // Addresses handed to rule inserts that have not come back as rows yet, so
  // several objects ticked in quick succession do not all compute the same
  // next free address from a table that has not changed.
  std::set<int> reserved_addresses_;

  std::vector<boost::signals2::scoped_connection> connections_;
  Cancelation cancelation_;
};
