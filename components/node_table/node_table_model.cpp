#include "components/node_table/node_table_model.h"

#include "base/executor.h"
#include "base/strings/utf_string_conversions.h"
#include "base/utils.h"
#include "core/event.h"
#include "model/node_id_util.h"
#include "model/scada_node_ids.h"
#include "node_service/node_promises.h"
#include "node_service/node_service.h"
#include "node_service/node_util.h"
#include "services/property_defs.h"
#include "services/task_manager.h"
#include "string_const.h"

using namespace std::chrono_literals;

namespace {

const char16_t kFetching[] = u"Загрузка...";
const auto kParentReferenceTypeId = scada::id::Organizes;
const auto kSortDelay = 300ms;

}  // namespace

NodeTableModel::NodeTableModel(std::shared_ptr<Executor> executor,
                               PropertyContext&& context)
    : PropertyContext{std::move(context)}, executor_{std::move(executor)} {
  row_model_.set_row_height(19);
}

NodeTableModel::~NodeTableModel() {
  node_service_.Unsubscribe(*this);
}

void NodeTableModel::SetParentNode(const NodeRef& parent_node) {
  cancelation_.Cancel();

  parent_node_ = parent_node;

  GetChildPropertyDefs(parent_node_)
      .then(cancelation_.Bind([this](const PropertyDefs& property_defs) {
        UpdateColumns(property_defs);
      }))
      // It's important to return promise form the callback.
      .then([parent_node] { return FetchChildren(parent_node); })
      .then(cancelation_.Bind([this] { UpdateRows(); }));
}

int NodeTableModel::GetRowCount() {
  return loading_ ? 1 : static_cast<int>(nodes_.size());
}

std::u16string NodeTableModel::GetRowTitle(int row) {
  if (loading_)
    return {};

  return base::UTF8ToUTF16(NodeIdToScadaString(nodes_[row].node_id()));
}

void NodeTableModel::GetCell(aui::GridCell& cell) {
  if (loading_) {
    cell.text = kFetching;
    cell.cell_color = aui::COLORREFToColor(::GetSysColor(COLOR_3DFACE));
    return;
  }

  assert(cell.row >= 0 && cell.row < (long)nodes_.size());
  assert(cell.column >= 0 && cell.column < column_model_.GetCount());

  const auto& node = nodes_[cell.row];
  auto& column = columns_[cell.column];

  if (column.attr_id == scada::AttributeId::NodeId) {
    cell.text = base::UTF8ToUTF16(NodeIdToScadaString(node.node_id()));
    cell.cell_color = aui::COLORREFToColor(::GetSysColor(COLOR_3DFACE));
  } else if (column.attr_id == scada::AttributeId::BrowseName)
    cell.text = base::UTF8ToUTF16(node.browse_name().name());
  else if (column.attr_id == scada::AttributeId::DisplayName)
    cell.text = node.display_name();
  else if (column.prop_def->IsReadOnly(node,
                                       column.property_declaration.node_id()))
    cell.cell_color = aui::COLORREFToColor(::GetSysColor(COLOR_3DFACE));
  else
    cell.text = column.prop_def->GetText(*this, node,
                                         column.property_declaration.node_id());
}

bool NodeTableModel::SetCellText(int row,
                                 int column,
                                 const std::u16string& text) {
  assert(row >= 0 && row < static_cast<int>(nodes_.size()));
  if (row < 0 || row >= static_cast<int>(nodes_.size()))
    return false;

  const auto& node = nodes_[row];
  auto& c = columns_[column];
  if (c.attr_id == scada::AttributeId::BrowseName) {
    task_manager_.PostUpdateTask(
        node.node_id(),
        scada::NodeAttributes().set_browse_name(base::UTF16ToUTF8(text)), {});
  } else if (c.attr_id == scada::AttributeId::DisplayName) {
    task_manager_.PostUpdateTask(
        node.node_id(),
        scada::NodeAttributes().set_display_name(scada::ToLocalizedText(text)),
        {});
  } else {
    c.prop_def->SetText(*this, node, c.property_declaration.node_id(), text);
  }
  return true;
}

aui::EditData NodeTableModel::GetEditData(int row, int column) {
  const auto& node = nodes_[row];
  assert(node);

  auto& c = columns_[column];
  if (c.attr_id == scada::AttributeId::NodeId)
    return {aui::EditData::EditorType::NONE};

  if (c.attr_id == scada::AttributeId::BrowseName ||
      c.attr_id == scada::AttributeId::DisplayName)
    return {aui::EditData::EditorType::TEXT};

  return c.prop_def->GetPropertyEditor(*this, node,
                                       c.property_declaration.node_id());
}

void NodeTableModel::UpdateRows() {
  loading_ = false;

  nodes_ = parent_node_.targets(kParentReferenceTypeId);

  // Subscribe only when nodes are loaded.
  node_service_.Subscribe(*this);

  Sort();
  NotifyModelChanged();
}

int NodeTableModel::FindRecord(const scada::NodeId& node_id) const {
  for (size_t i = 0; i < nodes_.size(); i++) {
    if (nodes_[i].node_id() == node_id)
      return static_cast<int>(i);
  }
  return -1;
}

void NodeTableModel::Update(const NodeRef& node) {
  int ix = FindRecord(node.node_id());
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

bool NodeTableModel::IsMatchingNode(const NodeRef& node) const {
  auto parent_ref = node.inverse_reference(scada::id::HierarchicalReferences);
  return parent_ref.target == parent_node_ &&
         IsSubtypeOf(parent_ref.reference_type, kParentReferenceTypeId);
}

void NodeTableModel::OnModelChanged(const scada::ModelChangeEvent& event) {
  if (event.verb & scada::ModelChangeEvent::NodeDeleted) {
    Delete(event.node_id);
  } else {
    auto node = node_service_.GetNode(event.node_id);
    if (IsMatchingNode(node))
      Update(node);
    else
      Delete(event.node_id);
  }
}

void NodeTableModel::OnNodeSemanticChanged(const scada::NodeId& node_id) {
  auto node = node_service_.GetNode(node_id);
  if (IsMatchingNode(node))
    Update(node);
}

void NodeTableModel::Sort() {
  sort_needed_ = false;

  if (sort_property_id_.is_null())
    return;

  struct CompareNodes {
    bool operator()(const NodeRef& left, const NodeRef& right) const {
      const auto& a = left[property_id].value().get_or(std::u16string());
      const auto& b = right[property_id].value().get_or(std::u16string());
      return HumanCompareText(a, b) < 0;
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

  executor_->PostDelayedTask(
      kSortDelay, cancelation_.Bind([this]() { ScheduleSortHelper(); }));
}

void NodeTableModel::ScheduleSortHelper() {
  sort_scheduled_ = false;

  if (sort_needed_)
    Sort();
}

void NodeTableModel::UpdateColumns(const PropertyDefs& property_defs) {
  columns_.clear();

  std::vector<aui::TableColumn> columns;

  columns.reserve(property_defs.size() + 2);

  // Browse name
  {
    columns_.push_back({scada::AttributeId::BrowseName});
    columns.emplace_back(
        aui::TableColumn{static_cast<int>(columns.size()),
                         std::u16string{kBrowseNameAttributeString}, 75,
                         aui::TableColumn::LEFT});
  }

  // Display name
  {
    columns_.push_back({scada::AttributeId::DisplayName});
    columns.emplace_back(
        aui::TableColumn{static_cast<int>(columns.size()),
                         std::u16string{kDisplayNameAttributeString}, 75,
                         aui::TableColumn::LEFT});
  }

  auto AddProp = [this, &columns](const NodeRef& property_declaration,
                                  const PropertyDefinition& def) {
    columns_.push_back({scada::AttributeId::Value, property_declaration, &def});
    int width = def.width() ? def.width() : 75;
    auto title = def.GetTitle(*this, property_declaration);
    columns.emplace_back(aui::TableColumn{static_cast<int>(columns.size()),
                                          title, width, def.alignment()});
  };

  for (auto& prop : property_defs) {
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
