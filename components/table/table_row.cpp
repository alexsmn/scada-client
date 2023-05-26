#include "components/table/table_row.h"

#include "base/format_time.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "client_utils.h"
#include "common/node_event_provider.h"
#include "components/table/table_model.h"
#include "controls/color.h"
#include "model/data_items_node_ids.h"
#include "node_service/node_util.h"
#include "services/profile.h"

int g_time_format = TIME_FORMAT_DATE | TIME_FORMAT_TIME | TIME_FORMAT_MSEC;

const int kValueFormat = FORMAT_DEFAULT;

namespace {

std::optional<aui::Color> GetNodeColor(const NodeRef& node,
                                       const scada::DataValue& data_value) {
  if (!IsInstanceOf(node, data_items::id::DiscreteItemType))
    return std::nullopt;

  if (data_value.value.is_null())
    return std::nullopt;

  int color_index = -1;

  bool bool_value = false;
  auto params = node.target(data_items::id::HasTsFormat);
  if (data_value.value.get(bool_value) && params) {
    auto pid = bool_value ? data_items::id::TsFormatType_CloseColor
                          : data_items::id::TsFormatType_OpenColor;
    color_index = params[pid].value().get_or(-1);
  }

  if (color_index >= 0 && color_index < static_cast<int>(aui::GetColorCount()))
    return aui::GetColor(color_index);

  return bool_value ? aui::ColorCode::Red : aui::ColorCode::Black;
}

std::u16string FormatCellTime(scada::DateTime time) {
  if (time.is_null())
    return std::u16string{};

  return base::UTF8ToUTF16(FormatTime(time, g_time_format));
}

}  // namespace

// TableRow

TableRow::TableRow(TableModel& model, int index)
    : model_{model}, index_{index}, Blinker{model.blinker_manager_} {
  timed_data_.node_modified_handler = [this] { NotifyUpdate(); };
  timed_data_.deletion_handler = [this] { NotifyUpdate(); };
  // Do not bind executor, since timed data interface is synchronous.
  timed_data_.property_change_handler = [this](const PropertySet& properties) {
    NotifyUpdate();
  };
  timed_data_.event_change_handler = [this] {
    SetBlinking(timed_data_.alerting());
  };
}

TableRow::~TableRow() = default;

std::string TableRow::GetFormula() const {
  return formula_.empty() ? std::string{} : '=' + formula_;
}

std::u16string TableRow::GetTitle() const {
  return timed_data_.GetTitle();
}

std::u16string TableRow::GetTooltip() const {
  return GetTimedDataTooltipText(timed_data_);
}

void TableRow::SetFormula(std::string formula, bool notify_update) {
  auto old_node_id = timed_data_.GetNode().node_id();

  formula_ = std::move(formula);
  if (!formula_.empty() && formula_[0] == '=')
    formula_.erase(formula_.begin());

  timed_data_.Connect(model_.timed_data_service(), formula_);

  SetBlinking(timed_data_.alerting());

  if (notify_update)
    NotifyUpdate();

  auto new_node_id = timed_data_.GetNode().node_id();

  if (old_node_id != new_node_id)
    model_.OnRowNodeChanged(old_node_id, new_node_id);
}

void TableRow::SetBlinking(bool blinking) {
  if (is_blinking_ == blinking)
    return;

  is_blinking_ = blinking;

  if (is_blinking_)
    Blinker::Start();
  else
    Blinker::Stop();

  NotifyUpdate();
}

void TableRow::NotifyUpdate() {
  model_.NotifyItemsChanged(index_, 1);
}

void TableRow::GetValueCell(TableCellEx& cell) const {
  const auto& data_value = timed_data_.current();
  cell.text = timed_data_.GetValueString(data_value.value, data_value.qualifier,
                                         kValueFormat);

  const auto& node = timed_data_.GetNode();
  if (auto color = GetNodeColor(node, data_value))
    cell.text_color = color.value();

  if (Blinker::GetState() && is_blinking_)
    cell.cell_color = aui::ColorCode::Yellow;
}

void TableRow::GetEventCell(TableCellEx& cell) const {
  // last unacked event
  const auto& node = timed_data_.GetNode();
  if (!node)
    return;

  const EventSet* events =
      model_.node_event_provider_.GetItemUnackedEvents(node.node_id());
  if (!events || events->empty())
    return;

  const scada::Event& last_event = **events->rbegin();
  cell.text = last_event.message;

  if (events->size() >= 2)
    cell.text.insert(0, base::StringPrintf(u"[%d] ", events->size()));
}

void TableRow::GetCellEx(TableCellEx& cell) const {
  cell.text.clear();

  if (cell.column_id == TableModel::COLUMN_TITLE)
    cell.cell_color = aui::Rgba{0xF8, 0xF8, 0xF8};

  switch (cell.column_id) {
    case TableModel::COLUMN_TITLE:
      cell.text = GetTitle();
      cell.icon_index = timed_data_.GetNode() ? 1 : -1;
      break;

    case TableModel::COLUMN_VALUE:
      GetValueCell(cell);
      break;

    case TableModel::COLUMN_SOURCE_TIMESTAMP:
      cell.text = FormatCellTime(timed_data_.current().source_timestamp);
      break;

    case TableModel::COLUMN_SERVER_TIMESTAMP:
      cell.text = FormatCellTime(timed_data_.current().server_timestamp);
      break;

    case TableModel::COLUMN_CHANGE_TIME:
      cell.text = FormatCellTime(timed_data_.change_time());
      break;

    case TableModel::COLUMN_EVENT:
      GetEventCell(cell);
      break;
  }

  if (cell.column_id != TableModel::COLUMN_TITLE) {
    const auto& data_value = timed_data_.current();
    if (data_value.qualifier.general_bad())
      cell.text_color = model_.profile_.bad_value_color;
  }
}

void TableRow::OnBlink(bool state) {
  NotifyUpdate();
}
