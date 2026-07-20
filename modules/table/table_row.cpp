#include "modules/table/table_row.h"

#include "aui/color.h"
#include "aui/severity_colors.h"
#include "aui/translation.h"
#include "base/format_time.h"
#include "base/u16format.h"
#include "base/utf_convert.h"
#include "events/event_set.h"
#include "events/node_event_provider.h"
#include "model/data_items_node_ids.h"
#include "modules/table/quality_mark.h"
#include "modules/table/sparkline.h"
#include "modules/table/table_model.h"
#include "node_service/node_util.h"
#include "profile/profile.h"
#include "ui/common/client_utils.h"

int g_time_format = TIME_FORMAT_DATE | TIME_FORMAT_TIME | TIME_FORMAT_MSEC;

const ValueFormat kValueFormat{FORMAT_DEFAULT};

namespace {

std::optional<scada::aui::Color> GetNodeColor(
    const NodeRef& node,
    const scada::DataValue& data_value) {
  if (!IsInstanceOf(node, scada::data_items::id::DiscreteItemType))
    return std::nullopt;

  if (data_value.value.is_null())
    return std::nullopt;

  int color_index = -1;

  bool bool_value = false;
  auto params = node.target(scada::data_items::id::HasTsFormat);
  if (data_value.value.get(bool_value) && params) {
    auto pid = bool_value ? scada::data_items::id::TsFormatType_CloseColor
                          : scada::data_items::id::TsFormatType_OpenColor;
    color_index = params[pid].value().get_or(-1);
  }

  if (color_index >= 0 &&
      color_index < static_cast<int>(scada::aui::GetColorCount()))
    return scada::aui::GetColor(color_index);

  if (bool_value)
    return scada::aui::ColorCode::Red;

  return std::nullopt;
}

std::u16string FormatCellTime(scada::DateTime time) {
  if (time.is_null())
    return std::u16string{};

  return UtfConvert<char16_t>(FormatTime(time, g_time_format));
}

// True when the opt-in reshell token theme is active; the quality-mark chrome
// (quality column, token value colouring) is gated on it so the legacy grid is
// unchanged.
bool ReshellActive() {
  return scada::aui::GetSeverityTheme() != scada::aui::SeverityTheme::kLegacy;
}

// The short, translated label shown in the quality column so quality is never
// signalled by colour alone (backlog cross-cutting acceptance).
std::u16string QualityLabel(scada::aui::Quality quality) {
  switch (quality) {
    case scada::aui::Quality::kBad:
      return Translate("Bad");
    case scada::aui::Quality::kUncertain:
      return Translate("Uncertain");
    case scada::aui::Quality::kGood:
      break;
  }
  return Translate("Good");
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
  // Historical values arriving for the sparkline window repaint the row.
  timed_data_.update_handler = [this](std::span<const scada::DataValue>) {
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
  auto old_node_id = timed_data_.node_id();

  formula_ = std::move(formula);
  if (!formula_.empty() && formula_[0] == '=')
    formula_.erase(formula_.begin());

  timed_data_.Connect(model_.timed_data_service(), formula_);

  // Under the reshell theme each row also observes a trailing history window
  // feeding its sparkline cell (the mockup's "Trend 1 h"); the legacy grid
  // stays current-only.
  if (ReshellActive())
    timed_data_.SetFrom(scada::base::Time::Now() - kSparklineWindow);

  SetBlinking(timed_data_.alerting());

  if (notify_update)
    NotifyUpdate();

  const auto& new_node_id = timed_data_.node_id();

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

  const auto& node = timed_data_.node();
  if (auto color = GetNodeColor(node, data_value))
    cell.text_color = color.value();

  if (Blinker::GetState() && is_blinking_)
    cell.cell_color = scada::aui::ColorCode::Yellow;
}

void TableRow::GetQualityCell(TableCellEx& cell) const {
  const scada::aui::Quality quality =
      QualityFromQualifier(timed_data_.current().qualifier);
  cell.text = QualityLabel(quality);
  if (auto color = scada::aui::QualityColor(quality))
    cell.text_color = *color;
}

void TableRow::GetEventCell(TableCellEx& cell) const {
  // last unacked event
  const auto& node_id = timed_data_.node_id();
  if (node_id.is_null())
    return;

  const EventSet* events =
      model_.node_event_provider_.GetItemUnackedEvents(node_id);
  if (!events || events->empty())
    return;

  const scada::Event& last_event = **events->rbegin();
  cell.text = last_event.message;

  if (events->size() >= 2)
    cell.text.insert(0, u16format(L"[{}] ", events->size()));
}

void TableRow::GetCellEx(TableCellEx& cell) const {
  cell.text.clear();

  switch (cell.column_id) {
    case TableModel::COLUMN_TITLE:
      cell.text = GetTitle();
      cell.icon_index = !timed_data_.node_id().is_null() ? 1 : -1;
      break;

    case TableModel::COLUMN_VALUE:
      GetValueCell(cell);
      break;

    case TableModel::COLUMN_QUALITY:
      GetQualityCell(cell);
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

  // Colour the row's data cells by quality. The quality column paints its own
  // colour (above); the title stays neutral. Under the reshell token theme use
  // the shared good/uncertain/bad ramp (uncertain rows read amber, bad rows
  // red, matching table-watch.html); otherwise keep the legacy single
  // bad-value colour so the default grid is unchanged.
  if (cell.column_id != TableModel::COLUMN_TITLE &&
      cell.column_id != TableModel::COLUMN_QUALITY) {
    const auto& data_value = timed_data_.current();
    if (ReshellActive()) {
      const scada::aui::Quality quality =
          QualityFromQualifier(data_value.qualifier);
      if (quality != scada::aui::Quality::kGood) {
        if (auto color = scada::aui::QualityColor(quality))
          cell.text_color = *color;
      }
    } else if (data_value.qualifier.general_bad()) {
      cell.text_color = model_.profile_.bad_value_color;
    }
  }
}

void TableRow::OnBlink(bool state) {
  NotifyUpdate();
}
