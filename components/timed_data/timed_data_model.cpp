#include "components/timed_data/timed_data_model.h"

#include "base/format_time.h"
#include "base/strings/utf_string_conversions.h"
#include "core/date_time.h"

// TimedDataModel

TimedDataModel::TimedDataModel(TimedDataModelContext&& context)
    : TimedDataModelContext{std::move(context)} {
  timed_data_.property_change_handler = [this](const PropertySet& properties) {
    if (properties.is_current_changed())
      Update({timed_data_.current().source_timestamp, scada::DateTime::Max()});
  };

  timed_data_.correction_handler = [this](size_t count,
                                          const scada::DataValue* tvqs) {
    assert(count > 0);
    Update({tvqs[0].source_timestamp, tvqs[count - 1].source_timestamp});
  };

  timed_data_.ready_handler = [this] {
    Update({timed_data_.ready_from(), scada::DateTime::Max()});
  };

  timed_data_.node_modified_handler = [this] { NotifyModelChanged(); };
}

void TimedDataModel::SetTimedData(TimedDataSpec timed_data) {
  timed_data_ = std::move(timed_data);
  Update({scada::DateTime::Min(), scada::DateTime::Max()});
}

void TimedDataModel::Update(scada::DateTimeRange range) {
  auto new_begin = begin_iterator_;

  int new_count = 0;
  if (timed_data_.connected() && timed_data_.values()) {
    base::span<const scada::DataValue> values = *timed_data_.values();
    new_begin = LowerBound(values, timed_data_.from());
    auto new_end = UpperBound(values, end_time_);
    new_count = new_end - new_begin;
  }

  if (new_count > count_) {
    int first = count_, count = new_count - count_;
    NotifyItemsAdding(first, count);
    begin_iterator_ = new_begin;
    count_ = new_count;
    NotifyItemsAdded(first, count);

  } else if (new_count < count_) {
    int first = new_count, count = count_ - new_count;
    NotifyItemsRemoving(first, count);
    begin_iterator_ = new_begin;
    count_ = new_count;
    NotifyItemsRemoved(first, count);
  }

  if (timed_data_.values()) {
    base::span<const scada::DataValue> values = *timed_data_.values();
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
  assert(timed_data_.values());
  auto& values = *timed_data_.values();
  return values[begin_iterator_ + row];
}

int TimedDataModel::GetRowCount() {
  return count_;
}

void TimedDataModel::GetCell(ui::TableCell& cell) {
  const auto& tvq = value(cell.row);

  switch (cell.column_id) {
    case CID_TIME:
      cell.text = ToString16(tvq.source_timestamp);
      break;

    case CID_VALUE:
      // Format without quality.
      cell.text = timed_data_.GetValueString(tvq.value, tvq.qualifier, 0);
      break;

    case CID_QUALITY:
      cell.text = ToString16(tvq.qualifier);
      break;

    case CID_COLLECTION_TIME:
      cell.text = base::UTF8ToUTF16(
          FormatTime(tvq.server_timestamp,
                     TIME_FORMAT_DATE | TIME_FORMAT_TIME | TIME_FORMAT_MSEC));
      break;
  }

  // highlight current time
  //	if (data && time == ((::Item*)data->rec)->val_time)
  //		cell.clrb = RGB(255, 255, 0);
}

void TimedDataModel::SetFormula(std::string_view formula) {
  TimedDataSpec timed_data;
  try {
    timed_data.SetFrom(base::Time::Now() - base::TimeDelta::FromHours(1));
    timed_data.Connect(timed_data_service_, formula);

  } catch (const std::exception&) {
  }

  SetTimedData(timed_data);
}

TimeRange TimedDataModel::GetTimeRange() const {
  return time_range_;
}

void TimedDataModel::SetTimeRange(const TimeRange& time_range) {
  time_range_ = time_range;
  auto [start, end] = GetTimeRangeBounds(time_range_);

  if (!timed_data_.connected())
    return;

  timed_data_.SetRange({start, end});

  end_time_ =
      time_range.end.is_null() ? scada::DateTime::Max() : time_range.end;

  Update({scada::DateTime::Min(), end_time_});
}
