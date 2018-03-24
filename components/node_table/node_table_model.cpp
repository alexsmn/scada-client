#include "components/node_table/node_table_model.h"

#include <set>

#include "base/bind.h"
#include "base/strings/sys_string_conversions.h"
#include "base/threading/sequenced_task_runner_handle.h"
#include "base/utils.h"
#include "common/node_id_util.h"
#include "common/node_service.h"
#include "common/node_util.h"
#include "common/scada_node_ids.h"
#include "services/property_defs.h"
#include "services/task_manager.h"
#include "skia/ext/skia_utils_win.h"

namespace {

const auto kSortDelay = base::TimeDelta::FromMilliseconds(300);

void GetTypeProperties(const NodeRef& type_definition,
                       std::set<scada::NodeId>& property_ids) {
  assert(type_definition.fetched());
  for (auto supertype_definition = type_definition; supertype_definition;
       supertype_definition = supertype_definition.supertype()) {
    for (auto& p : supertype_definition.properties())
      property_ids.emplace(p.id());
    for (auto& r : supertype_definition.references()) {
      if (!IsSubtypeOf(r.reference_type, scada::id::HasProperty))
        property_ids.emplace(r.reference_type.id());
    }
  }
}

void GetAllSubtypesProperties(const NodeRef& type_definition,
                              std::set<scada::NodeId>& property_ids) {
  assert(type_definition.fetched());
  GetTypeProperties(type_definition, property_ids);
  for (auto& subtype_definition :
       type_definition.targets(scada::id::HasSubtype)) {
    GetAllSubtypesProperties(subtype_definition, property_ids);
  }
}

PropertyDefs GetChildPropertyDefs(const NodeRef& parent_node) {
  assert(parent_node.fetched());

  std::set<NodeRef> child_type_definitions;
  for (auto node_type = parent_node.type_definition(); node_type;
       node_type = node_type.supertype()) {
    for (auto& component_decl : node_type.components())
      child_type_definitions.emplace(component_decl.type_definition());
  }

  std::set<scada::NodeId> property_ids;
  for (auto& child_type_definition : child_type_definitions)
    GetAllSubtypesProperties(child_type_definition, property_ids);

  PropertyDefs result;
  for (auto& p : property_ids) {
    if (auto* def = GetPropertyDef(p))
      result.emplace_back(p, def);
  }

  std::sort(result.begin(), result.end());

  return result;
}

}  // namespace

NodeTableModel::NodeTableModel(NodeService& node_service,
                               TaskManager& task_manager)
    : PropertyContext{node_service, task_manager}, node_service_{node_service} {
  row_model_.set_row_height(19);
}

NodeTableModel::~NodeTableModel() {
  SetFetchedParentNode(nullptr);
}

void NodeTableModel::SetParentNode(const NodeRef& parent_node) {
  auto weak_ptr = weak_ptr_factory_.GetWeakPtr();
  parent_node.Fetch(NodeFetchStatus::NodeAndChildren(),
                    [weak_ptr](const NodeRef& node) {
                      if (auto* ptr = weak_ptr.get())
                        ptr->SetFetchedParentNode(node);
                    });
}

void NodeTableModel::SetFetchedParentNode(const NodeRef& parent_node) {
  if (parent_node_ == parent_node)
    return;

  if (parent_node_) {
    node_service_.Unsubscribe(*this);
    parent_node_ = nullptr;
  }

  parent_node_ = parent_node;

  if (parent_node_)
    node_service_.Subscribe(*this);

  Update();
}

int NodeTableModel::GetRowCount() {
  return static_cast<int>(nodes_.size());
}

base::string16 NodeTableModel::GetRowTitle(int row) {
  return base::SysNativeMBToWide(NodeIdToScadaString(nodes_[row].id()));
}

void NodeTableModel::GetCell(ui::GridCell& cell) {
  assert(cell.row >= 0 && cell.row < (long)nodes_.size());
  assert(cell.column >= 0 && cell.column < column_model_.GetCount());

  const auto& node = nodes_[cell.row];
  auto& column = columns_[cell.column];

  if (column.attr_id == scada::AttributeId::BrowseName)
    cell.text = base::SysNativeMBToWide(node.browse_name().name());
  else if (column.attr_id == scada::AttributeId::DisplayName)
    cell.text = node.display_name();
  else if (column.prop_def->IsReadOnly(node, column.prop_decl_id))
    cell.cell_color = skia::COLORREFToSkColor(::GetSysColor(COLOR_3DFACE));
  else
    cell.text = column.prop_def->GetText(*this, node, column.prop_decl_id);
}

bool NodeTableModel::SetCellText(int row,
                                 int column,
                                 const base::string16& text) {
  assert(row >= 0 && row < nodes_.size());
  if (row < 0 || row >= nodes_.size())
    return false;

  const auto& node = nodes_[row];
  auto& c = columns_[column];
  if (c.attr_id == scada::AttributeId::BrowseName) {
    task_manager_.PostUpdateTask(
        node.id(),
        scada::NodeAttributes().set_browse_name(base::SysWideToNativeMB(text)),
        {});
  } else if (c.attr_id == scada::AttributeId::DisplayName) {
    task_manager_.PostUpdateTask(
        node.id(),
        scada::NodeAttributes().set_display_name(scada::ToLocalizedText(text)),
        {});
  } else {
    c.prop_def->SetText(*this, node, c.prop_decl_id, text);
  }
  return true;
}

PropertyEditor NodeTableModel::GetCellEditor(int row, int column) {
  const auto& node = nodes_[row];
  assert(node);
  auto& c = columns_[column];
  if (c.attr_id == scada::AttributeId::BrowseName ||
      c.attr_id == scada::AttributeId::DisplayName)
    return PropertyEditor{PropertyEditor::SIMPLE};
  const auto& type_definition = node.type_definition();
  return type_definition ? c.prop_def->GetPropertyEditor(*this, type_definition,
                                                         c.prop_decl_id)
                         : PropertyEditor{PropertyEditor::NONE};
}

void NodeTableModel::Update() {
  columns_.clear();
  nodes_.clear();

  if (parent_node_) {
    for (const auto& node : parent_node_.organizes())
      nodes_.push_back(node);

    InitColumns();
  }

  Sort();
}

int NodeTableModel::FindRecord(const scada::NodeId& node_id) const {
  for (size_t i = 0; i < nodes_.size(); i++) {
    if (nodes_[i].id() == node_id)
      return static_cast<int>(i);
  }
  return -1;
}

void NodeTableModel::Update(const NodeRef& node) {
  int ix = FindRecord(node.id());
  if (ix != -1) {
    NotifyRowsChanged(ix, 1);
  } else {
    nodes_.push_back(node);
    NotifyRowsAdded(nodes_.size() - 1, 1);
  }

  ScheduleSort();
}

void NodeTableModel::Delete(const scada::NodeId& node_id) {
  int ix = FindRecord(node_id);
  if (ix != -1) {
    nodes_.erase(nodes_.begin() + ix);
    NotifyModelChanged();
  }
}

void NodeTableModel::OnModelChanged(const scada::ModelChangeEvent& event) {
  if (event.verb & scada::ModelChangeEvent::NodeDeleted) {
    Delete(event.node_id);
  } else {
    auto node = node_service_.GetNode(event.node_id);
    if (node.parent() == parent_node_)
      Update(node);
    else
      Delete(event.node_id);
  }
}

void NodeTableModel::OnNodeSemanticChanged(const scada::NodeId& node_id) {
  auto node = node_service_.GetNode(node_id);
  if (node.parent() == parent_node_)
    Update(node);
}

void NodeTableModel::Sort() {
  sort_needed_ = false;

  if (sort_property_id_.is_null())
    return;

  struct CompareNodes {
    bool operator()(const NodeRef& left, const NodeRef& right) const {
      const auto& a = left[property_id].value().get_or(std::string());
      const auto& b = right[property_id].value().get_or(std::string());
      return a < b;
    }

    const scada::NodeId property_id;
  };

  std::sort(nodes_.begin(), nodes_.end(), CompareNodes{sort_property_id_});

  NotifyModelChanged();
}

void NodeTableModel::ScheduleSort() {
  if (sort_property_id_.is_null())
    return;

  sort_needed_ = true;

  if (sort_scheduled_)
    return;

  sort_scheduled_ = true;

  base::SequencedTaskRunnerHandle::Get()->PostDelayedTask(
      FROM_HERE,
      base::Bind(&NodeTableModel::ScheduleSortHelper,
                 weak_ptr_factory_.GetWeakPtr()),
      kSortDelay);
}

void NodeTableModel::ScheduleSortHelper() {
  sort_scheduled_ = false;

  if (sort_needed_)
    Sort();
}

void NodeTableModel::InitColumns() {
  auto properties = GetChildPropertyDefs(parent_node_);

  std::vector<ui::TableColumn> columns;

  columns.reserve(properties.size() + 1);

  // Display name
  {
    columns_.push_back(
        {scada::AttributeId::DisplayName, scada::NodeId(), nullptr});
    columns.emplace_back(columns.size(), L"Имя", 75, ui::TableColumn::LEFT);
  }

  auto AddProp = [this, &columns](const scada::NodeId& prop_id,
                                  const PropertyDefinition& def) {
    columns_.push_back({scada::AttributeId::Value, prop_id, &def});
    int width = def.width() ? def.width() : 75;
    auto title = def.GetTitle(*this, prop_id);
    columns.emplace_back(columns.size(), title, width, def.alignment());
  };

  for (auto& prop : properties) {
    if (auto* hier_prop = prop.second->AsHierarchical()) {
      for (auto& p : hier_prop->children())
        AddProp(prop.first, *p);
    } else {
      AddProp(prop.first, *prop.second);
    }
  }
  column_model_.SetColumns(columns.size(), columns.data());
}

void NodeTableModel::SetSorting(const scada::NodeId& property_id) {
  if (sort_property_id_ == property_id)
    return;

  sort_property_id_ = property_id;
  Sort();
}
