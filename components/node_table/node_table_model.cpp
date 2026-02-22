#include "components/node_table/node_table_model.h"

#include "base/executor.h"
#include "base/range_util.h"
#include "base/utf_convert.h"
#include "base/utils.h"
#include "scada/event.h"
#include "model/node_id_util.h"
#include "model/scada_node_ids.h"
#include "node_service/node_promises.h"
#include "node_service/node_service.h"
#include "node_service/node_util.h"
#include "properties/property_definition.h"
#include "properties/property_service.h"
#include "services/task_manager.h"
#include "string_const.h"

#include <boost/range/adaptor/transformed.hpp>

using namespace std::chrono_literals;

namespace {

const char16_t kFetching[] = u"Loading...";
const auto kParentReferenceTypeId = scada::id::Organizes;
const auto kSortDelay = 300ms;

}  // namespace

NodeTableModel::NodeTableModel(std::shared_ptr<Executor> executor,
                               PropertyService& property_service,
                               PropertyContext&& context)
    : PropertyContext{std::move(context)},
      executor_{std::move(executor)},
      property_service_{property_service} {
  row_model_.set_row_height(19);
}

NodeTableModel::~NodeTableModel() {
  node_service_.Unsubscribe(*this);
}

void NodeTableModel::SetParentNode(const NodeRef& parent_node) {
  cancelation_.Cancel();

  parent_node_ = parent_node;

  property_service_.GetChildPropertyDefs(parent_node_)
      .then(cancelation_.Bind([this](const PropertyDefs& property_defs) {
        UpdateColumns(property_defs);
      }))
      // It's important to return promise form the callback.
      .then([parent_node] { return FetchChildren(parent_node); })
      .then(cancelation_.Bind([this] { UpdateRows(); }));
}

int NodeTableModel::GetRowCount() {
  return loading_ ? 1 : static_cast<int>(rows_.size());
}

std::u16string NodeTableModel::GetRowTitle(int row) {
  if (loading_)
    return {};

  return UtfConvert<char16_t>(NodeIdToScadaString(rows_[row].node.node_id()));
}

void NodeTableModel::GetCell(aui::GridCell& cell) {
  if (loading_) {
    cell.text = kFetching;
    cell.cell_color = aui::COLORREFToColor(::GetSysColor(COLOR_3DFACE));
    return;
  }

  assert(cell.row >= 0 && cell.row < static_cast<int>(rows_.size()));
  assert(cell.column >= 0 && cell.column < column_model_.GetCount());

  const auto& node = rows_[cell.row].node;
  const auto& column = columns_[cell.column];

  if (column.attr_id == scada::AttributeId::NodeId) {
    cell.text = UtfConvert<char16_t>(NodeIdToScadaString(node.node_id()));
    cell.cell_color = aui::COLORREFToColor(::GetSysColor(COLOR_3DFACE));
  } else if (column.attr_id == scada::AttributeId::BrowseName)
    cell.text = UtfConvert<char16_t>(node.browse_name().name());
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
  assert(row >= 0 && row < static_cast<int>(rows_.size()));
  if (row < 0 || row >= static_cast<int>(rows_.size()))
    return false;

  const auto& node = rows_[row].node;

  if (const auto& c = columns_[column];
      c.attr_id == scada::AttributeId::BrowseName) {
    task_manager_.PostUpdateTask(node.node_id(),
                                 {.browse_name = UtfConvert<char>(text)}, {});
  } else if (c.attr_id == scada::AttributeId::DisplayName) {
    task_manager_.PostUpdateTask(
        node.node_id(), {.display_name = scada::ToLocalizedText(text)}, {});
  } else {
    c.prop_def->SetText(*this, node, c.property_declaration.node_id(), text);
  }

  return true;
}

aui::EditData NodeTableModel::GetEditData(int row, int column) {
  const auto& node = rows_[row].node;
  assert(node);

  const auto& c = columns_[column];
  if (c.attr_id == scada::AttributeId::NodeId)
    return {.editor_type = aui::EditData::EditorType::NONE};

  if (c.attr_id == scada::AttributeId::BrowseName ||
      c.attr_id == scada::AttributeId::DisplayName)
    return {.editor_type = aui::EditData::EditorType::TEXT};

  return c.prop_def->GetPropertyEditor(*this, node,
                                       c.property_declaration.node_id());
}

void NodeTableModel::UpdateRows() {
  loading_ = false;

  auto nodes = parent_node_.targets(kParentReferenceTypeId);

  rows_.clear();
  for (const auto& node : nodes) {
    auto& row = rows_.emplace_back(node);
    FetchRow(row);
  }

  // Subscribe only when nodes are loaded.
  node_service_.Subscribe(*this);

  Sort();
  NotifyModelChanged();
}

int NodeTableModel::FindRowIndex(const scada::NodeId& node_id) const {
  for (size_t i = 0; i < rows_.size(); i++) {
    const auto& node = rows_[i].node;
    // E.g. TS format can update.
    if (node.node_id() == node_id) {
      return static_cast<int>(i);
    }
  }
  return -1;
}

std::vector<std::pair<int, int>> NodeTableModel::FindUpdatedRanges(
    const scada::NodeId& node_id) const {
  std::vector<std::pair<int /*first*/, int /*last*/>> results;
  for (int i = 0; i < static_cast<int>(rows_.size()); i++) {
    const auto& row = rows_[i];
    // E.g. TS format can update.
    if (row.node.node_id() == node_id ||
        base::Contains(row.additional_targets, node_id)) {
      if (!results.empty() && results.back().second + 1 == i) {
        results.back().second = i;
      } else {
        results.emplace_back(i, i);
      }
    }
  }
  return results;
}

void NodeTableModel::FetchRow(Row& row) const {
  row.node.Fetch();

  row.additional_targets.clear();
  std::ranges::for_each(columns_, [&](const auto& column) {
    if (column.prop_def) {
      column.prop_def->GetAdditionalTargets(
          row.node, column.property_declaration.node_id(),
          row.additional_targets);
    }
  });

  std::ranges::for_each(row.additional_targets, [&](const auto& target_id) {
    node_service_.GetNode(target_id).Fetch();
  });
}

void NodeTableModel::Update(const NodeRef& node) {
  if (int ix = FindRowIndex(node.node_id()); ix != -1) {
    FetchRow(rows_[ix]);
    NotifyRowsChanged(ix, 1);
  } else {
    auto& row = rows_.emplace_back(node);
    FetchRow(row);
    NotifyRowsAdded(static_cast<int>(rows_.size()) - 1, 1);
  }

  ScheduleSort();
}

void NodeTableModel::Delete(const scada::NodeId& node_id) {
  if (int ix = FindRowIndex(node_id); ix != -1) {
    rows_.erase(rows_.begin() + ix);
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
    return;
  }

  auto node = node_service_.GetNode(event.node_id);
  if (IsMatchingNode(node))
    Update(node);
  else
    Delete(event.node_id);
}

void NodeTableModel::OnNodeSemanticChanged(const scada::NodeId& node_id) {
  auto node = node_service_.GetNode(node_id);
  if (IsMatchingNode(node))
    Update(node);

  UpdatedReferencingNodes(node_id);
}

void NodeTableModel::UpdatedReferencingNodes(const scada::NodeId& node_id) {
  if (auto ranges = FindUpdatedRanges(node_id); !ranges.empty()) {
    for (const auto& [first, last] : ranges) {
      NotifyRowsChanged(first, last - first + 1);
    }
    ScheduleSort();
  }
}

void NodeTableModel::Sort() {
  sort_needed_ = false;

  if (sort_property_id_.is_null())
    return;

  struct CompareRows {
    bool operator()(const Row& left, const Row& right) const {
      const auto& a = left.node[property_id].value().get_or(std::u16string());
      const auto& b = right.node[property_id].value().get_or(std::u16string());
      return HumanCompareText(a, b) < 0;
    }

    const scada::NodeId property_id;
  };

  std::ranges::sort(rows_, CompareRows{sort_property_id_});

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
    columns_.emplace_back(scada::AttributeId::BrowseName);
    columns.emplace_back(static_cast<int>(columns.size()),
                         kBrowseNameAttributeString, 75,
                         aui::TableColumn::LEFT);
  }

  // Display name
  {
    columns_.emplace_back(scada::AttributeId::DisplayName);
    columns.emplace_back(static_cast<int>(columns.size()),
                         kDisplayNameAttributeString, 75,
                         aui::TableColumn::LEFT);
  }

  auto AddProp = [this, &columns](const NodeRef& property_declaration,
                                  const PropertyDefinition& def) {
    columns_.emplace_back(scada::AttributeId::Value, property_declaration,
                          &def);
    int width = def.width() ? def.width() : 75;
    auto title = def.GetTitle(*this, property_declaration);
    columns.emplace_back(static_cast<int>(columns.size()), title, width,
                         def.alignment());
  };

  for (const auto& prop : property_defs) {
    if (const auto* hier_prop = prop.second->AsHierarchical()) {
      for (const auto& p : hier_prop->children())
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
