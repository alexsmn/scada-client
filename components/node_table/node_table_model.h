#pragma once

#include "base/memory/weak_ptr.h"
#include "common/node_observer.h"
#include "common/node_ref.h"
#include "services/property_defs.h"
#include "ui/base/models/fixed_row_model.h"
#include "ui/base/models/grid_model.h"

class NodeService;
class PropertyDefinition;

class NodeTableModel : private PropertyContext,
                       public ui::GridModel,
                       private views::FixedRowModel::Delegate,
                       public NodeRefObserver {
 public:
  NodeTableModel(NodeService& node_service, TaskManager& task_manager);
  virtual ~NodeTableModel() override;

  const NodeRef& parent_node() const { return parent_node_; }
  void SetParentNode(const NodeRef& parent_node);

  views::FixedRowModel& row_model() { return row_model_; }
  ui::ColumnHeaderModel& column_model() { return column_model_; }

  typedef std::vector<NodeRef> Nodes;
  const Nodes& nodes() const { return nodes_; }

  PropertyEditor GetCellEditor(int row, int column);

  const scada::NodeId& sort_property_id() const { return sort_property_id_; }
  void SetSorting(const scada::NodeId& property_id);

  // GridModel
  virtual void GetCell(ui::GridCell& cell) override;
  virtual bool SetCellText(int row,
                           int column,
                           const base::string16& text) override;

 private:
  void SetFetchedParentNode(const NodeRef& parent_node);

  void Update();

  void InitColumns();

  void Update(const NodeRef& node);
  void Delete(const scada::NodeId& node_id);
  int FindRecord(const scada::NodeId& node_id) const;

  void ScheduleSort();
  void ScheduleSortHelper();
  void Sort();

  // views::FixedRowModel::Delegate
  virtual int GetRowCount() override;
  virtual base::string16 GetRowTitle(int row) override;

  // NodeRefObserver
  virtual void OnModelChanged(const scada::ModelChangeEvent& event) override;
  virtual void OnNodeSemanticChanged(const scada::NodeId& node_id) override;

  views::FixedRowModel row_model_{*this};
  ui::ColumnHeaderModel column_model_;

  NodeService& node_service_;
  NodeRef parent_node_;

  Nodes nodes_;

  struct Column {
    scada::AttributeId attr_id;
    scada::NodeId prop_decl_id;
    const PropertyDefinition* prop_def;
  };

  std::vector<Column> columns_;

  bool sort_scheduled_ = false;
  bool sort_needed_ = false;
  scada::NodeId sort_property_id_;

  base::WeakPtrFactory<NodeTableModel> weak_ptr_factory_{this};
};
