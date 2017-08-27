#include "components/table/table_model.h"

#include "base/format_time.h"
#include "base/time/time.h"
#include "base/utils.h"
#include "base/color.h"
#include "client_utils.h"
#include "common_resources.h"
#include "services/profile.h"
#include "components/table/table_row.h"
#include "common/scada_node_ids.h"
#include "common/event_manager.h"
#include "common/node_ref_util.h"

int g_time_format = TIME_FORMAT_DATE | TIME_FORMAT_TIME | TIME_FORMAT_MSEC;

#ifdef UI_VIEWS
const int kValueFormat = FORMAT_DEFAULT | FORMAT_COLOR;
#else
const int kValueFormat = FORMAT_DEFAULT;
#endif

bool TableModel::RowsComparer::operator()(
    const TableRow* left,
    const TableRow* right) const {
  if (!left && !right)
    return false;
  if (!left)
    return false;
  if (!right)
    return true;
    
  switch (command_id_) {
    case ID_SORT_NAME:
      return HumanCompareText(
          base::SysWideToNativeMB(left->GetTitle()).c_str(),
          base::SysWideToNativeMB(right->GetTitle()).c_str()) < 0;
    
    case ID_SORT_CHANNEL: {
      auto item1 = left->timed_data().GetNode();
      auto item2 = right->timed_data().GetNode();
      if (!item1 || !item2)
        return item2 != nullptr;
      const auto& type_id1 = item1.type_definition().id();
      const auto& type_id2 = item2.type_definition().id();
      if (type_id1 != type_id2)
        return type_id1 < type_id2;
      auto channel1 = item1[id::DataItemType_Input1].value().get_or(std::string{});
      auto channel2 = item2[id::DataItemType_Input1].value().get_or(std::string{});
      return channel1 < channel2;
    }
    
    default:
      return false;
  }
}

TableModel::TableModel(TimedDataService& timed_data_service, events::EventManager& event_manager, Profile& profile)
    : timed_data_service_(timed_data_service),
      event_manager_(event_manager),
      profile_(profile) {
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

  if (cell.row == rows_.size())
    return;

  const TableRow* trow = rows_[cell.row];
  if (!trow) {
    //cell.clrb = RGB(218, 218, 218);
    return;
  }

  auto _item = trow->timed_data().GetNode();
  const auto& value = trow->timed_data().current();

  switch (cell.column_id) {
    case COLUMN_TITLE: {
      cell.text = trow->GetTitle();
      cell.image_index = 1;
      break;
    }

    case COLUMN_VALUE:
      cell.text = trow->timed_data().GetValueString(value.value, value.qualifier, kValueFormat);
      if (IsInstanceOf(_item, id::DiscreteItemType)) {
        if (!value.value.is_null()) {
          auto params = _item.target(id::AnalogItemType_DisplayFormat);
          int color_index = -1;
          bool bool_value;
          if (value.value.get(bool_value) && params) {
            auto pid = bool_value ? id::TsFormatType_CloseColor : id::TsFormatType_OpenColor;
            color_index = params[pid].value().get_or(-1);
          }
          if (color_index >= 0 && color_index < palette::GetColorCount())
            cell.text_color = palette::GetColor(color_index);
          else
            cell.text_color = bool_value ? SK_ColorRED : SK_ColorBLACK;
        }
      }
      if (Blinker::GetState() && trow->is_blinking())
        cell.cell_color = SK_ColorYELLOW;
      break;

    case COLUMN_UPDATE_TIME:
      if (!value.time.is_null())
        cell.text = base::SysNativeMBToWide(FormatTime(value.time, g_time_format));
      break;

    case COLUMN_CHANGE_TIME: {
      base::Time time = trow->timed_data().change_time();
      if (!time.is_null())
        cell.text = base::SysNativeMBToWide(FormatTime(time, g_time_format));
      break;
    }

    case COLUMN_EVENT:
      // last unacked event
      if (_item) {
        const events::EventSet* events = event_manager_.GetItemUnackedEvents(_item.id());
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
    cell.text_color = profile_.bad_value_color();
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
  for (RowSet::iterator i = blinking_rows_.begin();
                        i != blinking_rows_.end(); ++i)
    (*i)->NotifyUpdate();
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

    scada::NodeId item_id = row->timed_data().trid();
    if (item_id != scada::NodeId())
      item_ids.insert(item_id);

    delete row;
  }

  rows_.erase(rows_.begin() + start, rows_.begin() + start + count);

  for (int i = start; i < (int)rows_.size(); ++i)
    rows_[i]->set_index(i);

  NotifyItemsRemoved(start, count);

  if (item_changed_) {
    for (NodeIdSet::const_iterator i = item_ids.begin(); i != item_ids.end(); ++i) {
      if (FindItem(*i) == -1)
        item_changed_(*i, false);
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

bool TableModel::SetFormula(int row, const std::string& formula) {
  if (row == -1)
    row = static_cast<int>(rows_.size());

  DCHECK(row <= (int)rows_.size());

  if (row == (int)rows_.size()) {
    rows_.push_back(new TableRow(*this, row));
    NotifyItemsAdded(row, 1);
  }

  DCHECK(rows_[row]);
  TableRow& trow = *rows_[row];

  scada::NodeId old_trid = trow.timed_data().trid();
  try {
    trow.SetFormula(formula);
  } catch (const std::exception&) {
    return false;
  }
  scada::NodeId new_trid = trow.timed_data().trid();

  if (item_changed_ && old_trid != new_trid) {
    if (old_trid != scada::NodeId() && FindItem(old_trid) == -1)
      item_changed_(old_trid, false);
    if (new_trid != scada::NodeId())
      item_changed_(new_trid, true);
  }

  NotifyItemsChanged(row, 1);

  return true;
}

int TableModel::FindItem(const scada::NodeId& trid) const {
  for (int i = 0; i < (int)rows_.size(); i++) {
    const TableRow* row = GetRow(i);
    if (row && row->timed_data().trid() == trid)
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
