#pragma once

#include "base/cancelation.h"
#include "node_service/node_observer.h"
#include "node_service/node_ref.h"
#include "services/property_defs.h"
#include "ui/base/models/fixed_row_model.h"
#include "ui/base/models/grid_model.h"

class Executor;
class NodeService;
class PropertyDefinition;

class NodeTableModel : private PropertyContext,
                       public ui::GridModel,
                       private views::FixedRowModel::Delegate,
                       public NodeRefObserver {
 public:
  NodeTableModel(std::shared_ptr<Executor> executor, PropertyContext&& context);
  virtual ~NodeTableModel() override;

  const NodeRef& parent_node() const { return parent_node_; }
  void SetParentNode(const NodeRef& parent_node);

  views::FixedRowModel& row_model() { return row_model_; }
  ui::ColumnHeaderModel& column_model() { return column_model_; }

  typedef std::vector<NodeRef> Nodes;
  const Nodes& nodes() const { return nodes_; }

  const scada::NodeId& sort_property_id() const { return sort_property_id_; }
  void SetSorting(const scada::NodeId& property_id);

  // GridModel
  virtual void GetCell(ui::GridCell& cell) override;
  virtual bool SetCellText(int row,
                           int column,
                           const std::u16string& text) override;
  virtual ui::EditData GetEditData(int row, int column) override;

  bool loading() const { return loading_; }

 private:
  void UpdateColumns(const PropertyDefs& property_defs);
  void UpdateParent();

  bool IsMatchingNode(const NodeRef& node) const;
  void Update(const NodeRef& node);
  void Delete(const scada::NodeId& node_id);
  int FindRecord(const scada::NodeId& node_id) const;

  void ScheduleSort();
  void ScheduleSortHelper();
  void Sort();

  // views::FixedRowModel::Delegate
  virtual int GetRowCount() override;
  virtual std::u16string GetRowTitle(int row) override;

  // NodeRefObserver
  virtual void OnModelChanged(const scada::ModelChangeEvent& event) override;
  virtual void OnNodeSemanticChanged(const scada::NodeId& node_id) override;

  const std::shared_ptr<Executor> executor_;

  views::FixedRowModel row_model_{*this};
  ui::ColumnHeaderModel column_model_;

  NodeRef parent_node_;

  Nodes nodes_;

  struct Column {
    scada::AttributeId attr_id;
    NodeRef property_declaration;
    const PropertyDefinition* prop_def;
  };

  std::vector<Column> columns_;

  bool loading_ = true;

  bool sort_scheduled_ = false;
  bool sort_needed_ = false;
  scada::NodeId sort_property_id_;

  Cancelation cancelation_;
};
