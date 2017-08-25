#pragma once

#include "base/memory/weak_ptr.h"
#include "services/property_defs.h"
#include "ui/base/models/grid_model.h"
#include "ui/base/models/fixed_row_model.h"
#include "common/node_ref.h"
#include "common/node_ref_observer.h"

class PropertyDefinition;
class TaskManager;

class NodeTableModel : public ui::GridModel,
                       private views::FixedRowModel::Delegate,
                       public NodeRefObserver {
 public:
  explicit NodeTableModel(PropertyContext& context);
  virtual ~NodeTableModel() override;

  const NodeRef& parent_node() const { return parent_node_; }
  void SetParentNode(NodeRef parent_node);
  void SetParentNodeId(const scada::NodeId& node_id);

  views::FixedRowModel& row_model() { return row_model_; }
  ui::ColumnHeaderModel& column_model() { return column_model_; }

  typedef std::vector<NodeRef> Nodes;
  const Nodes& nodes() const { return nodes_; }

  PropertyEditor GetCellEditor(int row, int column);

  void Update();

  // GridModel
  virtual void GetCell(ui::GridCell& cell) override;
  virtual bool SetCellText(int row, int column, const base::string16& text) override;

 private:
  void SetNodes(const std::vector<NodeRef>& nodes);

  void InitColumns();

  void Update(const scada::NodeId& node_id);
  void Insert(const NodeRef& node);
  void Delete(const scada::NodeId& node_id);
  int Find(const scada::NodeId& node_id) const;

  void Sort();

  // views::FixedRowModel::Delegate
  virtual int GetRowCount() override;
  virtual base::string16 GetRowTitle(int row) override;

  // NodeRefObserver events
  virtual void OnNodeAdded(const scada::NodeId& node_id) override;
  virtual void OnNodeDeleted(const scada::NodeId& node_id) override;
  virtual void OnReferenceAdded(const scada::NodeId& node_id) override;
  virtual void OnReferenceDeleted(const scada::NodeId& node_id) override;
  virtual void OnNodeSemanticChanged(const scada::NodeId& node_id) override;

  PropertyContext context_;

  views::FixedRowModel row_model_;
  ui::ColumnHeaderModel column_model_;

  NodeRef parent_node_;

  Nodes nodes_;

  struct Column {
    scada::AttributeId attr_id;
    scada::NodeId prop_decl_id;
    const PropertyDefinition* prop_def;
  };

  std::vector<Column> columns_;

  bool sorting_locked_ = false;

  base::WeakPtrFactory<NodeTableModel> weak_ptr_factory_{this};
};
