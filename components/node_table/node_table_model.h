#pragma once

#include "base/cancelation.h"
#include "controls/models/fixed_row_model.h"
#include "controls/models/grid_model.h"
#include "node_service/node_observer.h"
#include "node_service/node_ref.h"
#include "services/property_defs.h"

class Executor;
class NodeService;
class PropertyDefinition;

class NodeTableModel : private PropertyContext,
                       public aui::GridModel,
                       private aui::FixedRowModel::Delegate,
                       public NodeRefObserver {
 public:
  NodeTableModel(std::shared_ptr<Executor> executor, PropertyContext&& context);
  virtual ~NodeTableModel() override;

  const NodeRef& parent_node() const { return parent_node_; }
  void SetParentNode(const NodeRef& parent_node);

  aui::FixedRowModel& row_model() { return row_model_; }
  aui::ColumnHeaderModel& column_model() { return column_model_; }

  NodeRef node(int index) const { return rows_[index].node; }

  const scada::NodeId& sort_property_id() const { return sort_property_id_; }
  void SetSorting(const scada::NodeId& property_id);

  // GridModel
  virtual void GetCell(aui::GridCell& cell) override;
  virtual bool SetCellText(int row,
                           int column,
                           const std::u16string& text) override;
  virtual aui::EditData GetEditData(int row, int column) override;

  bool loading() const { return loading_; }

 private:
  struct Row {
    NodeRef node;
    std::vector<scada::NodeId> additional_targets;
  };

  void FetchRow(Row& row) const;

  void UpdateColumns(const PropertyDefs& property_defs);
  void UpdateRows();
  void UpdatedReferencingNodes(const scada::NodeId& node_id);
  void PrefetchNodes(std::span<const NodeRef> nodes);

  bool IsMatchingNode(const NodeRef& node) const;
  void Update(const NodeRef& node);
  void Delete(const scada::NodeId& node_id);
  int FindRowIndex(const scada::NodeId& node_id) const;
  std::vector<std::pair<int, int>> FindUpdatedRanges(
      const scada::NodeId& node_id) const;

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

  aui::FixedRowModel row_model_{*this};
  aui::ColumnHeaderModel column_model_;

  NodeRef parent_node_;

  struct Column {
    scada::AttributeId attr_id;
    NodeRef property_declaration;
    const PropertyDefinition* prop_def;
  };

  std::vector<Column> columns_;

  std::vector<Row> rows_;

  bool loading_ = true;

  bool sort_scheduled_ = false;
  bool sort_needed_ = false;
  scada::NodeId sort_property_id_;

  Cancelation cancelation_;
};
