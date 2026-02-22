#include "components/table/table_model.h"

#include "aui/translation.h"
#include "base/utf_convert.h"
#include "base/time/time.h"
#include "base/utils.h"
#include "common_resources.h"
#include "components/table/table_row.h"
#include "controller/node_id_set.h"
#include "model/data_items_node_ids.h"
#include "aui/dialog_service.h"

namespace {

bool CompareRows(const TableRow* left,
                 const TableRow* right,
                 unsigned command_id) {
  if (!left && !right)
    return false;
  if (!left)
    return false;
  if (!right)
    return true;

  switch (command_id) {
    case ID_SORT_NAME:
      return HumanCompareText(left->GetTitle(), right->GetTitle()) < 0;

    case ID_SORT_CHANNEL: {
      auto node1 = left->timed_data().node();
      auto node2 = right->timed_data().node();
      if (!node1 || !node2) {
        return node2 != nullptr;
      }
      const auto& type_id1 = node1.type_definition().node_id();
      const auto& type_id2 = node2.type_definition().node_id();
      if (type_id1 != type_id2) {
        return type_id1 < type_id2;
      }
      auto channel1 = node1[data_items::id::DataItemType_Input1].value().get_or(
          std::string{});
      auto channel2 = node2[data_items::id::DataItemType_Input1].value().get_or(
          std::string{});
      return channel1 < channel2;
    }

    default:
      return false;
  }
}

}  // namespace

// TableModel

TableModel::TableModel(TableModelContext&& context)
    : TableModelContext{std::move(context)} {}

TableModel::~TableModel() = default;

void TableModel::GetCellEx(TableCellEx& cell) const {
  assert(cell.row >= 0 && cell.row <= (long)rows_.size());

  cell.text.clear();

  if (cell.column_id == TableModel::COLUMN_TITLE)
    cell.cell_color = aui::Rgba{0xF8, 0xF8, 0xF8};

  if (cell.row == static_cast<int>(rows_.size())) {
    if (cell.column_id == 0) {
      cell.text = Translate("Enter expression");
      cell.text_color = aui::Rgba{192, 192, 192};
    }
    return;
  }

  const auto& trow = rows_[cell.row];
  if (!trow) {
    // cell.clrb = RGB(218, 218, 218);
    return;
  }

  trow->GetCellEx(cell);
}

int TableModel::GetRowCount() {
  return static_cast<int>(rows_.size()) + 1;
}

void TableModel::GetCell(aui::TableCell& cell) {
  TableCellEx cell_ex;
  static_cast<aui::TableCell&>(cell_ex) = cell;
  GetCellEx(cell_ex);
  cell = cell_ex;
}

void TableModel::Clear() {
  DeleteRows(0, rows_.size());
}

bool TableModel::DeleteRows(int start, int count) {
  assert(start >= 0);
  assert(count >= 0);

  if (start >= (int)rows_.size())
    return false;

  count = (std::min)(count, (int)rows_.size() - start);

  if (count == 0)
    return false;

  NodeIdSet node_ids;
  for (int i = 0; i < count; ++i) {
    auto& row = rows_[start + i];
    if (!row)
      continue;

    if (auto node_id = row->timed_data().node_id(); !node_id.is_null()) {
      node_ids.emplace(std::move(node_id));
    }

    row.reset();
  }

  NotifyItemsRemoving(start, count);

  rows_.erase(rows_.begin() + start, rows_.begin() + start + count);

  for (int i = start; i < (int)rows_.size(); ++i)
    rows_[i]->set_index(i);

  NotifyItemsRemoved(start, count);

  if (item_changed_) {
    for (auto& node_id : node_ids) {
      if (FindItem(node_id) == -1) {
        item_changed_(node_id, false);
      }
    }
  }

  return true;
}

int TableModel::MoveRow(int row, bool up) {
  int row2 = row;
  if (up)
    row2--;
  else
    row2++;

  if (row2 < 0 || row2 >= (int)rows_.size())
    return -1;

  std::swap(rows_[row], rows_[row2]);
  if (rows_[row])
    rows_[row]->set_index(row);
  if (rows_[row2])
    rows_[row2]->set_index(row2);

  NotifyItemsChanged(row, 1);
  NotifyItemsChanged(row2, 1);

  return row2;
}

bool TableModel::SetFormula(int row, std::string formula) {
  if (row == -1)
    row = static_cast<int>(rows_.size());

  int added_first = static_cast<int>(rows_.size());
  int added_count = row - added_first + 1;
  if (added_count > 0) {
    NotifyItemsAdding(added_first, added_count);
    for (int i = 0; i < added_count; ++i)
      rows_.push_back(std::make_unique<TableRow>(*this, added_first + i));
  }

  assert(rows_[row]);
  TableRow& trow = *rows_[row];

  try {
    trow.SetFormula(std::move(formula), false);
  } catch (const std::exception&) {
    return false;
  }

  if (added_count > 0) {
    // Must be coherent with |NotifyItemsAdding| above.
    NotifyItemsAdded(added_first, added_count);
  } else {
    NotifyItemsChanged(row, 1);
  }

  return true;
}

int TableModel::FindItem(const scada::NodeId& node_id) const {
  for (int i = 0; i < (int)rows_.size(); i++) {
    const TableRow* row = GetRow(i);
    if (row && row->timed_data().node_id() == node_id)
      return i;
  }
  return -1;
}

void TableModel::Sort(unsigned command_id) {
  std::sort(rows_.begin(), rows_.end(),
            [command_id](const auto& a, const auto& b) {
              return CompareRows(a.get(), b.get(), command_id);
            });

  NotifyItemsChanged(0, row_count());
}

std::u16string TableModel::GetTooltip(int row, int column_id) {
  if (column_id != TableModel::COLUMN_TITLE)
    return std::u16string();

  const TableRow* trow = GetRow(row);
  if (!trow)
    return std::u16string();

  return trow->GetTooltip();
}

TableRow* TableModel::GetRow(int index) {
  assert(index >= 0 && index <= (int)rows_.size());
  if (index == static_cast<int>(rows_.size()))
    return nullptr;
  return rows_[index].get();
}

const TableRow* TableModel::GetRow(int index) const {
  return const_cast<TableModel*>(this)->GetRow(index);
}

bool TableModel::SetCellText(int row,
                             int column_id,
                             const std::u16string& text) {
  assert(column_id == TableModel::COLUMN_TITLE);

  std::string text2 = UtfConvert<char>(text);
  if (!text2.empty() && text2[0] == L'=')
    text2.erase(0, 1);

  if (!SetFormula(row, text2)) {
    dialog_service_.RunMessageBox(Translate("Invalid expression."), {},
                                  MessageBoxMode::Error);
    return false;
  }

  return true;
}

bool TableModel::IsEditable(int row, int column) {
  return column == 0;
}

void TableModel::OnRowNodeChanged(const scada::NodeId& old_node_id,
                                  const scada::NodeId& new_node_id) {
  if (!item_changed_)
    return;

  if (!old_node_id.is_null() && FindItem(old_node_id) == -1)
    item_changed_(old_node_id, false);

  if (!new_node_id.is_null())
    item_changed_(new_node_id, true);
}
