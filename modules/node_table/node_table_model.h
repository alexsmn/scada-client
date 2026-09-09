#pragma once

#include "base/any_executor.h"
#include "base/lifetime.h"

#include "aui/models/fixed_row_model.h"
#include "aui/models/grid_model.h"
#include "base/cancelation.h"
#include "node_service/node_ref.h"
#include "properties/property_context.h"

#include <boost/signals2/connection.hpp>
#include <memory>
#include <span>

class NodeService;
class PropertyDefinition;
class PropertyService;

class NodeTableModel : private PropertyContext,
                       public scada::aui::GridModel,
                       private scada::aui::FixedRowModel::Delegate,
                       public std::enable_shared_from_this<NodeTableModel> {
 public:
  NodeTableModel(AnyExecutor executor,
                 PropertyService& property_service,
                 PropertyContext&& context);
  virtual ~NodeTableModel() override;

  const NodeRef& parent_node() const SCADA_LIFETIME_BOUND {
    return parent_node_;
  }
  void SetParentNode(const NodeRef& parent_node);

  scada::aui::FixedRowModel& row_model() SCADA_LIFETIME_BOUND {
    return row_model_;
  }
  scada::aui::ColumnHeaderModel& column_model() SCADA_LIFETIME_BOUND {
    return column_model_;
  }

  NodeRef node(int index) const {
    return index < static_cast<int>(rows_.size()) ? rows_[index].node
                                                  : NodeRef();
  }

  const scada::NodeId& sort_property_id() const SCADA_LIFETIME_BOUND {
    return sort_property_id_;
  }
  void SetSorting(const scada::NodeId& property_id);

  // GridModel
  virtual void GetCell(scada::aui::GridCell& cell) override;
  virtual bool SetCellText(int row,
                           int column,
                           const std::u16string& text) override;
  virtual scada::aui::EditData GetEditData(int row, int column) override;

  bool loading() const { return loading_; }

 private:
  using PropertyDefs =
      std::vector<std::pair<NodeRef /*prop_decl*/, const PropertyDefinition*>>;

  struct Row {
    NodeRef node;
    std::vector<scada::NodeId> additional_targets;
  };

  void FetchRow(Row& row) const;

  void UpdateColumns(const PropertyDefs& property_defs);
  void UpdateRows();
  void UpdatedReferencingNodes(const scada::NodeId& node_id);

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

  void OnModelChanged(const scada::ModelChangeEvent& event);
  void OnNodeSemanticChanged(const scada::NodeId& node_id);

  const AnyExecutor executor_;
  PropertyService& property_service_;

  scada::aui::FixedRowModel row_model_{*this};
  scada::aui::ColumnHeaderModel column_model_;

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

  boost::signals2::scoped_connection model_changed_connection_;
  boost::signals2::scoped_connection node_semantic_changed_connection_;
};
