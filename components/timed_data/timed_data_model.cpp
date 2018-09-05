#include "components/timed_data/timed_data_model.h"

#include "base/format_time.h"
#include "base/strings/sys_string_conversions.h"
#include "base/table_writer.h"
#include "core/date_time.h"

#include <fstream>

// TimedDataModel

TimedDataModel::TimedDataModel(TimedDataModelContext&& context)
    : TimedDataModelContext{std::move(context)} {
  timed_data_.property_change_handler =
      [this](const rt::PropertySet& properties) {
        if (properties.is_current_changed())
          Update();
      };

  timed_data_.correction_handler = [this](size_t count,
                                          const scada::DataValue* tvqs) {
    assert(count > 0);
    Update();
  };

  timed_data_.ready_handler = [this] { Update(); };
  timed_data_.node_modified_handler = [this] { NotifyModelChanged(); };
}

void TimedDataModel::SetTimedData(rt::TimedDataSpec timed_data) {
  timed_data_ = std::move(timed_data);
  Update();
}

void TimedDataModel::Update() {
  int old_count = count_;
  count_ = 0;
  if (timed_data_.connected() && timed_data_.values()) {
    auto& values = *timed_data_.values();
    begin_iterator_ = rt::LowerBound(values, timed_data_.from());
    auto end_iterator_ =
        end_time_.is_null() ? values.end() : rt::UpperBound(values, end_time_);
    count_ = std::distance(begin_iterator_, end_iterator_);
  }

  if (count_ > old_count)
    NotifyItemsAdded(old_count, count_ - old_count);
  else if (count_ < old_count)
    NotifyModelChanged();
}

const scada::DataValue& TimedDataModel::value(int row) const {
  assert(row <= count_);
  auto i = begin_iterator_ + row;
  return *i;
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
      cell.text = base::SysNativeMBToWide(
          FormatTime(tvq.server_timestamp,
                     TIME_FORMAT_DATE | TIME_FORMAT_TIME | TIME_FORMAT_MSEC));
      break;
  }

  // highlight current time
  //	if (data && time == ((::Item*)data->rec)->val_time)
  //		cell.clrb = RGB(255, 255, 0);
}

void TimedDataModel::SetFormula(const std::string& formula) {
  rt::TimedDataSpec timed_data;
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

  timed_data_.SetFrom(start);
  end;

  end_time_ = time_range.end;

  Update();
}

void TimedDataModel::ExportToCsv(const std::filesystem::path& path) {
  std::wofstream stream{path};

  // https://stackoverflow.com/questions/11610583/wostream-fails-to-output-wstring
  stream.imbue(
      std::locale(stream.getloc(), new std::codecvt_utf8_utf16<wchar_t>));

  TableWriter writer{stream};

  writer.StartRow();
  writer.WriteCell(L"Время");
  writer.WriteCell(L"Значение");
  writer.WriteCell(L"Качество");
  writer.WriteCell(L"Время приема");

  for (int i = 0; i < GetRowCount(); ++i) {
    const auto& row = this->value(i);

    writer.StartRow();
    writer.WriteCell(ToString16(row.source_timestamp));
    writer.WriteCell(timed_data_.GetValueString(row.value, row.qualifier, 0));
    writer.WriteCell(ToString16(row.qualifier));
    writer.WriteCell(ToString16(row.server_timestamp));
  }
}
