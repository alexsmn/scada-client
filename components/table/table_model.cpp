#include "components/table/table_model.h"

#include "base/format_time.h"
#include "base/time/time.h"
#include "base/utils.h"
#include "client_utils.h"
#include "common/event_fetcher.h"
#include "node_service/node_util.h"
#include "common_resources.h"
#include "components/table/table_row.h"
#include "controls/color.h"
#include "model/data_items_node_ids.h"
#include "model/scada_node_ids.h"
#include "services/dialog_service.h"
#include "services/profile.h"

int g_time_format = TIME_FORMAT_DATE | TIME_FORMAT_TIME | TIME_FORMAT_MSEC;

#ifdef UI_VIEWS
const int kValueFormat = FORMAT_DEFAULT | FORMAT_COLOR;
#else
const int kValueFormat = FORMAT_DEFAULT;
#endif

bool TableModel::RowsComparer::operator()(const TableRow* left,
                                          const TableRow* right) const {
  if (!left && !right)
    return false;
  if (!left)
    return false;
  if (!right)
    return true;

  switch (command_id_) {
    case ID_SORT_NAME:
      return HumanCompareText(left->GetTitle(), right->GetTitle()) < 0;

    case ID_SORT_CHANNEL: {
      auto item1 = left->timed_data().GetNode();
      auto item2 = right->timed_data().GetNode();
      if (!item1 || !item2)
        return item2 != nullptr;
      const auto& type_id1 = item1.type_definition().node_id();
      const auto& type_id2 = item2.type_definition().node_id();
      if (type_id1 != type_id2)
        return type_id1 < type_id2;
      auto channel1 = item1[data_items::id::DataItemType_Input1].value().get_or(
          std::string{});
      auto channel2 = item2[data_items::id::DataItemType_Input1].value().get_or(
          std::string{});
      return channel1 < channel2;
    }

    default:
      return false;
  }
}

TableModel::TableModel(TableModelContext&& context)
    : TableModelContext{std::move(context)} {
  Blinker::Start();
}

TableModel::~TableModel() {
  for (size_t i = 0; i < rows_.size(); ++i)
    delete rows_[i];
}

void TableModel::GetCellEx(CellEx& cell) {
  assert(cell.row >= 0 && cell.row <= (long)rows_.size());

  cell.text.clear();

  if (cell.column_id == COLUMN_TITLE)
    cell.cell_color = SkColorSetRGB(0xF8, 0xF8, 0xF8);

  if (cell.row == rows_.size()) {
    if (cell.column_id == 0) {
      cell.text = L"Введите выражение";
      cell.text_color = SkColorSetRGB(192, 192, 192);
    }
    return;
  }

  const TableRow* trow = rows_[cell.row];
  if (!trow) {
    // cell.clrb = RGB(218, 218, 218);
    return;
  }

  auto node = trow->timed_data().GetNode();
  const auto& value = trow->timed_data().current();

  switch (cell.column_id) {
    case COLUMN_TITLE: {
      cell.text = trow->GetTitle();
      cell.image_index = 1;
      break;
    }

    case COLUMN_VALUE:
      cell.text = trow->timed_data().GetValueString(
          value.value, value.qualifier, kValueFormat);
      if (IsInstanceOf(node, data_items::id::DiscreteItemType)) {
        if (!value.value.is_null()) {
          auto params = node.target(data_items::id::HasTsFormat);
          int color_index = -1;
          bool bool_value;
          if (value.value.get(bool_value) && params) {
            auto pid = bool_value ? data_items::id::TsFormatType_CloseColor
                                  : data_items::id::TsFormatType_OpenColor;
            color_index = params[pid].value().get_or(-1);
          }
          if (color_index >= 0 && color_index < aui::GetColorCount())
            cell.text_color = aui::GetColor(color_index).sk_color();
          else
            cell.text_color = bool_value ? SK_ColorRED : SK_ColorBLACK;
        }
      }
      if (Blinker::GetState() && trow->is_blinking())
        cell.cell_color = SK_ColorYELLOW;
      break;

    case COLUMN_UPDATE_TIME:
      if (!value.source_timestamp.is_null())
        cell.text = base::SysNativeMBToWide(
            FormatTime(value.source_timestamp, g_time_format));
      break;

    case COLUMN_CHANGE_TIME: {
      base::Time time = trow->timed_data().change_time();
      if (!time.is_null())
        cell.text = base::SysNativeMBToWide(FormatTime(time, g_time_format));
      break;
    }

    case COLUMN_EVENT:
      // last unacked event
      if (node) {
        const EventSet* events =
            event_fetcher_.GetItemUnackedEvents(node.node_id());
        if (events && !events->empty()) {
          const scada::Event& event = **events->rbegin();
          cell.text = event.message;
          if (events->size() >= 2)
            cell.text.insert(0, base::StringPrintf(L"[%d] ", events->size()));
        }
      }
      break;
  }

  if (cell.column_id != COLUMN_TITLE && value.qualifier.general_bad())
    cell.text_color = profile_.bad_value_color;
}

int TableModel::GetRowCount() {
  return static_cast<int>(rows_.size()) + 1;
}

void TableModel::GetCell(ui::TableCell& cell) {
  CellEx cell_ex;
  static_cast<ui::TableCell&>(cell_ex) = cell;
  cell_ex.image_index = -1;
  GetCellEx(cell_ex);
  cell = cell_ex;
}

void TableModel::OnBlink(bool state) {
  for (auto* row : blinking_rows_)
    row->NotifyUpdate();
}

void TableModel::Clear() {
  DeleteRows(0, rows_.size());

  assert(blinking_rows_.empty());
}

bool TableModel::DeleteRows(int start, int count) {
  assert(start >= 0);
  assert(count >= 0);

  if (start >= (int)rows_.size())
    return false;

  count = (std::min)(count, (int)rows_.size() - start);

  if (count == 0)
    return false;

  NodeIdSet item_ids;
  for (int i = 0; i < count; ++i) {
    TableRow* row = rows_[start + i];
    if (!row)
      continue;

    auto item_id = row->timed_data().GetNode().node_id();
    if (!item_id.is_null())
      item_ids.insert(item_id);

    delete row;
  }

  NotifyItemsRemoving(start, count);

  rows_.erase(rows_.begin() + start, rows_.begin() + start + count);

  for (int i = start; i < (int)rows_.size(); ++i)
    rows_[i]->set_index(i);

  NotifyItemsRemoved(start, count);

  if (item_changed_) {
    for (auto& item_id : item_ids) {
      if (FindItem(item_id) == -1)
        item_changed_(item_id, false);
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
      rows_.push_back(new TableRow(*this, added_first + i));
    NotifyItemsAdded(added_first, added_count);
  }

  assert(rows_[row]);
  TableRow& trow = *rows_[row];

  auto old_node_id = trow.timed_data().GetNode().node_id();
  try {
    trow.SetFormula(std::move(formula));
  } catch (const std::exception&) {
    return false;
  }
  auto new_node_id = trow.timed_data().GetNode().node_id();

  if (item_changed_ && old_node_id != new_node_id) {
    if (!old_node_id.is_null() && FindItem(old_node_id) == -1)
      item_changed_(old_node_id, false);
    if (!new_node_id.is_null())
      item_changed_(new_node_id, true);
  }

  NotifyItemsChanged(row, 1);

  return true;
}

int TableModel::FindItem(const scada::NodeId& node_id) const {
  for (int i = 0; i < (int)rows_.size(); i++) {
    const TableRow* row = GetRow(i);
    if (row && row->timed_data().GetNode().node_id() == node_id)
      return i;
  }
  return -1;
}

void TableModel::Sort(unsigned command_id) {
  blinking_rows_.clear();

  std::sort(rows_.begin(), rows_.end(), RowsComparer(command_id));

  NotifyItemsChanged(0, row_count());
}

base::string16 TableModel::GetTooltip(int row, int column_id) {
  if (column_id != COLUMN_TITLE)
    return base::string16();

  const TableRow* trow = GetRow(row);
  if (!trow)
    return base::string16();

  return GetTimedDataTooltipText(trow->timed_data());
}

TableRow* TableModel::GetRow(int index) {
  assert(index >= 0 && index <= (int)rows_.size());
  if (index == rows_.size())
    return NULL;
  return rows_[index];
}

const TableRow* TableModel::GetRow(int index) const {
  return const_cast<TableModel*>(this)->GetRow(index);
}

bool TableModel::SetCellText(int row,
                             int column_id,
                             const base::string16& text) {
  assert(column_id == TableModel::COLUMN_TITLE);

  std::string text2 = base::SysWideToNativeMB(text);
  if (!text2.empty() && text2[0] == L'=')
    text2.erase(0, 1);

  if (!SetFormula(row, text2)) {
    dialog_service_.RunMessageBox(L"Неверное выражение.", {},
                                  MessageBoxMode::Error);
    return false;
  }

  return true;
}

bool TableModel::IsEditable(int row, int column) {
  return column == 0;
}
