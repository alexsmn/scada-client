#include "components/node_table/node_table_model.h"

#include <set>

#include "base/auto_reset.h"
#include "base/strings/sys_string_conversions.h"
#include "base/utils.h"
#include "services/property_defs.h"
#include "services/task_manager.h"
#include "common/scada_node_ids.h"
#include "skia/ext/skia_utils_win.h"
#include "common/node_ref_service.h"
#include "common/node_ref_util.h"
#include "common/browse_util.h"

namespace {

struct TypeInfo {
  std::vector<NodeRef> property_declarations;
  std::vector<NodeRef> reference_types;
};

void GetTypeInfo(const NodeRef& type_definition, TypeInfo& type_info) {
  for (auto base_type = type_definition; base_type; base_type = base_type.supertype()) {
    for (auto& p : base_type.properties())
      type_info.property_declarations.emplace_back(p);
    for (auto& r : base_type.references())
      type_info.reference_types.emplace_back(r.reference_type);
  }
}

void GetTypeInfoRecursive(const NodeRef& type_definition, TypeInfo& type_info) {
  /*for (auto& ref : scada::FilterReferences(type.forward_references(), OpcUaId_HasSubtype)) {
    if (auto* subtype = scada::AsTypeDefinition(ref.node))
      GetInheritedTypesProperties(*subtype, property_ids);
  }*/
  GetTypeInfo(type_definition, type_info);
}

PropertyDefs GetChildrenPropertyDefs(const NodeRef& parent_node) {
  /*std::set<NodeRef> node_types;
  for (auto node_type = parent_node.type_definition(); node_type;
            node_type = node_type.supertype()) {
    for (auto& component_decl : node_type.components())
      node_types.emplace(component_decl.type_definition());
  }

  std::set<scada::NodeId> property_ids;
  for (auto& node_type : node_types)
    GetInheritedTypesProperties(node_type, property_ids);

  PropertyDefs result;
  for (auto& p : property_ids) {
    auto* def = GetPropertyDef(p);
    if (def)
      result.emplace_back(p, def);
  }

  std::sort(result.begin(), result.end());
  return result;*/

  return PropertyDefs{};
}

} // namespace

NodeTableModel::NodeTableModel(PropertyContext& context)
    : context_(context),
      row_model_(*this) {
  row_model_.set_row_height(19);

  context_.node_service_.AddObserver(*this);
}

NodeTableModel::~NodeTableModel() {
  SetParentNode(nullptr);

  context_.node_service_.RemoveObserver(*this);
}

void NodeTableModel::SetParentNode(NodeRef parent_node) {
  if (parent_node_ == parent_node)
    return;

  parent_node_ = parent_node;
}

void NodeTableModel::SetParentNodeId(const scada::NodeId& node_id) {
  auto weak_ptr = weak_ptr_factory_.GetWeakPtr();
  context_.node_service_.GetNode(node_id).Fetch([weak_ptr](NodeRef node) {
    if (!node.status())
      return;
    if (auto* ptr = weak_ptr.get())
      ptr->SetParentNode(node);
  });
}

int NodeTableModel::GetRowCount() {
  return static_cast<int>(nodes_.size());
}

base::string16 NodeTableModel::GetRowTitle(int row) {
  return base::SysNativeMBToWide(nodes_[row].id().ToString());
}

void NodeTableModel::GetCell(ui::GridCell& cell) {
  assert(cell.row >= 0 && cell.row < (long)nodes_.size());
  assert(cell.column >= 0 && cell.column < column_model_.GetCount());

  const auto& node = nodes_[cell.row];
  auto& column = columns_[cell.column];

  if (column.attr_id == OpcUa_Attributes_BrowseName)
    cell.text = base::SysNativeMBToWide(node.browse_name().name());
  else if (column.prop_def->IsReadOnly(node, column.prop_decl_id))
    cell.cell_color = skia::COLORREFToSkColor(::GetSysColor(COLOR_3DFACE));
  else
    cell.text = column.prop_def->GetText(context_, node, column.prop_decl_id);
}

bool NodeTableModel::SetCellText(int row, int column, const base::string16& text) {
  const auto& node = nodes_[row];
  auto& c = columns_[column];
  if (c.attr_id == OpcUa_Attributes_BrowseName) {
    context_.task_manager_.PostUpdateTask(node.id(),
        scada::NodeAttributes().set_browse_name(scada::QualifiedName{base::SysWideToNativeMB(text), 0}) , {});
  } else {
    c.prop_def->SetText(context_, node, c.prop_decl_id, text);
  }
  return true;
}

PropertyEditor NodeTableModel::GetCellEditor(int row, int column) {
  const auto& node = nodes_[row];
  assert(node);
  auto& c = columns_[column];
  if (c.attr_id == OpcUa_Attributes_BrowseName)
    return PropertyEditor(PropertyEditor::SIMPLE);
  const auto& type_definition = node.type_definition();
  return type_definition ?
      c.prop_def->GetPropertyEditor(context_, type_definition, c.prop_decl_id) :
      PropertyEditor(PropertyEditor::NONE);
}

void NodeTableModel::Update() {
  columns_.clear();
  nodes_.clear();
  NotifyModelChanged();

  if (parent_node_) {
    auto weak_ptr = weak_ptr_factory_.GetWeakPtr();
    BrowseNodes(context_.node_service_,
        {parent_node_.id(), scada::BrowseDirection::Forward, OpcUaId_Organizes, true},
        [weak_ptr](const scada::Status& status, std::vector<NodeRef> nodes) {
          if (!status)
            return;
          if (auto* ptr = weak_ptr.get())
            ptr->SetNodes(std::move(nodes));
        });
  }
}

void NodeTableModel::SetNodes(const std::vector<NodeRef>& nodes) {
  nodes_ = std::move(nodes);
  Sort();

  // TODO: Update model.

  InitColumns();
}

void NodeTableModel::Update(const scada::NodeId& node_id) {
  // TODO: Can be moved.

  int i = Find(node_id);
  if (i != -1)
    NotifyRowsChanged(i, 1);
}

int NodeTableModel::Find(const scada::NodeId& node_id) const {
  // TODO: Map.
  for (int i = 0; i < static_cast<int>(nodes_.size()); i++)
    if (nodes_[i].id() == node_id)
      return i;
  return -1;
}

void NodeTableModel::Insert(const NodeRef& node) {
  assert(node);

  int ix = Find(node.id());
  if (ix != -1) {
    NotifyRowsChanged(ix, 1);
    return;
  }

  nodes_.push_back(node);
  Sort();

  NotifyModelChanged();
}

void NodeTableModel::Delete(const scada::NodeId& node_id) {
  int ix = Find(node_id);
  if (ix != -1) {
    nodes_.erase(nodes_.begin() + ix);
    NotifyModelChanged();
  }
}

void NodeTableModel::OnNodeAdded(const scada::NodeId& node_id) {
  // if (parent_node_.node_ptr() == scada::GetParent(node))
  //   Insert(node);
  // TODO: Handle addition.
}

void NodeTableModel::OnNodeDeleted(const scada::NodeId& node_id) {
  Delete(node_id);
}

void NodeTableModel::OnNodeSemanticChanged(const scada::NodeId& node_id) {
  Update(node_id);
}

void NodeTableModel::OnReferenceAdded(const scada::NodeId& node_id) {
  Update(node_id);
}

void NodeTableModel::OnReferenceDeleted(const scada::NodeId& node_id) {
  Update(node_id);
}

void NodeTableModel::Sort() {
  if (sorting_locked_)
    return;
    
  struct CompareRecords {
    bool operator()(const NodeRef& left, const NodeRef& right) const {
      auto left_data_item = IsInstanceOf(left, id::DataItemType);
      auto right_data_item = IsInstanceOf(right, id::DataItemType);
      auto left_path = left_data_item ? left[id::DataItemType_Input1].value().get_or(std::string{}) : std::string();
      auto right_path = right_data_item ? right[id::DataItemType_Input1].value().get_or(std::string{}) : std::string();
      if (left_path.empty() && right_path.empty())
        return left.id() < right.id();
      return HumanCompareText(left_path.c_str(), right_path.c_str()) < 0;
    }
  };
    
  std::sort(nodes_.begin(), nodes_.end(), CompareRecords());
}

void NodeTableModel::InitColumns() {
  auto properties = GetChildrenPropertyDefs(parent_node_);

  std::vector<ui::TableColumn> columns;

  columns.reserve(properties.size() + 1);

  // Display name
  {
    columns_.push_back({ OpcUa_Attributes_BrowseName, scada::NodeId(), nullptr });
    columns.emplace_back(columns.size(), L"Èìÿ", 75, ui::TableColumn::LEFT);
  }

  auto AddProp = [this, &columns](const NodeRef& prop_decl, const PropertyDefinition& def) {
      columns_.push_back({ OpcUa_Attributes_Value, prop_decl.id(), &def });
      int width = def.width() ? def.width() : 75;
      auto title = base::SysNativeMBToWide(prop_decl.display_name().text());
      columns.emplace_back(columns.size(), std::move(title), width, def.alignment());
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
