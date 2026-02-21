#include "components/timed_data/timed_data_model.h"

#include "base/format_time.h"
#include "base/utf_convert.h"
#include "base/time/clock.h"
#include "common/data_value_traits.h"
#include "common/timed_data_util.h"
#include "profile/window_definition.h"
#include "scada/date_time.h"
#include "timed_data/timed_data_property.h"

// TimedDataModel

TimedDataModel::TimedDataModel(TimedDataModelContext&& context)
    : TimedDataModelContext{std::move(context)} {
  timed_data_.property_change_handler = [this](const PropertySet& properties) {
    if (properties.is_current_changed()) {
      UpdateRows(
          {timed_data_.current().source_timestamp, scada::DateTime::Max()});
    }
  };

  timed_data_.update_handler =
      [this](std::span<const scada::DataValue> values) {
        if (!values.empty()) {
          UpdateRows({values.front().source_timestamp,
                      values.back().source_timestamp});
        }
      };

  timed_data_.ready_handler = [this] {
    UpdateRows({timed_data_.ready_from(), scada::DateTime::Max()});
  };

  timed_data_.node_modified_handler = [this] { NotifyModelChanged(); };
}

void TimedDataModel::Init(const WindowDefinition& window_def) {
  if (const WindowItem* item = window_def.FindItem("Item")) {
    SetFormula(item->GetString("path"));
  }

  SetTimeRange(RestoreTimeRange(window_def).value_or(TimeRange::Type::Day));
}

void TimedDataModel::UpdateRows(const scada::DateTimeRange& range) {
  auto new_begin = begin_iterator_;

  int new_count = 0;
  if (timed_data_.connected()) {
    const auto& values = timed_data_.values();
    new_begin = LowerBound(values, timed_data_.from());
    auto new_end = UpperBound(values, end_time_);
    new_count = new_end - new_begin;
  }

  if (new_count > count_) {
    int first = count_;
    int count = new_count - count_;
    NotifyItemsAdding(first, count);
    begin_iterator_ = new_begin;
    count_ = new_count;
    NotifyItemsAdded(first, count);

  } else if (new_count < count_) {
    int first = new_count;
    int count = count_ - new_count;
    NotifyItemsRemoving(first, count);
    begin_iterator_ = new_begin;
    count_ = new_count;
    NotifyItemsRemoved(first, count);
  }

  {
    const auto& values = timed_data_.values();
    int start = LowerBound(values, timed_data_.from());
    int first = LowerBound(values, range.first);
    int last = UpperBound(values, range.second);
    first = std::max(first, start);
    last = std::min(last, start + count_);
    NotifyItemsChanged(first - start, last - first + 1);
  }
}

const scada::DataValue& TimedDataModel::value(int row) const {
  assert(row < count_);
  return timed_data_.values()[begin_iterator_ + row];
}

int TimedDataModel::GetRowCount() {
  return count_;
}

void TimedDataModel::GetCell(aui::TableCell& cell) {
  const auto& data_value = value(cell.row);

  switch (cell.column_id) {
    case CID_TIME:
      cell.text = UtfConvert<char16_t>(FormatTime(
          data_value.source_timestamp, TIME_FORMAT_DATE | TIME_FORMAT_TIME |
                                           TIME_FORMAT_MSEC |
                                           (utc_time_ ? TIME_FORMAT_UTC : 0)));
      break;

    case CID_VALUE:
      // Format without quality.
      cell.text = timed_data_.GetValueString(
          data_value.value, data_value.qualifier, ValueFormat{0});
      break;

    case CID_QUALITY:
      cell.text = ToString16(data_value.qualifier);
      break;

    case CID_COLLECTION_TIME:
      cell.text = UtfConvert<char16_t>(FormatTime(
          data_value.server_timestamp, TIME_FORMAT_DATE | TIME_FORMAT_TIME |
                                           TIME_FORMAT_MSEC |
                                           (utc_time_ ? TIME_FORMAT_UTC : 0)));
      break;
  }

  // highlight current time
  //	if (data && time == ((::Item*)data->rec)->val_time)
  //		cell.clrb = RGB(255, 255, 0);
}

void TimedDataModel::SetFormula(std::string_view formula) {
  TimedDataSpec timed_data;
  try {
    timed_data.SetFrom(timed_data_.from());
    timed_data.Connect(timed_data_service_, formula);

  } catch (const std::exception&) {
  }

  timed_data_ = timed_data;
  SetTimeRange(time_range_);
  UpdateRows({scada::DateTime::Min(), scada::DateTime::Max()});
}

TimeRange TimedDataModel::GetTimeRange() const {
  return time_range_;
}

void TimedDataModel::SetTimeRange(const TimeRange& time_range) {
  time_range_ = time_range;
  auto [start, end] = ToDateTimeRange(time_range_, clock_.Now());

  timed_data_.SetRange({start, end});

  end_time_ =
      time_range.end.is_null() ? scada::DateTime::Max() : time_range.end;

  UpdateRows({scada::DateTime::Min(), end_time_});
}
