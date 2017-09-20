#include "components/timed_data/timed_data_model.h"

#include "base/strings/sys_string_conversions.h"
#include "base/format_time.h"

std::string FormatQuality(scada::Qualifier qualifier) {
  std::string text;
  if (qualifier.bad())
    text += "Недост ";
  if (qualifier.backup())
    text += "Резерв ";
  if (qualifier.offline())
    text += "НетСвязи ";
  if (qualifier.manual())
    text += "Ручной ";
  if (qualifier.misconfigured())
    text += "НеСконф ";
  if (qualifier.simulated())
    text += "Эмулирован ";
  if (qualifier.sporadic())
    text += "Спорадика ";
  if (qualifier.stale())
    text += "Устарел ";
  if (qualifier.failed())
    text += "Ошибка ";
  return text;
}

// TimedDataModel

TimedDataModel::TimedDataModel(TimedDataService& timed_data_service)
    : timed_data_service_(timed_data_service),
      cached_index_(-1),
      count_(0) {
  timed_data_.set_delegate(this);
}

void TimedDataModel::SetTimedData(rt::TimedDataSpec timed_data) {
  timed_data_ = std::move(timed_data);
  Update();
}

void TimedDataModel::Update() {
  cached_index_ = -1;

  int old_count = count_;
  count_ = 0;
  if (timed_data_.connected() && timed_data_.values()) {
    begin_iterator_ = timed_data_.values()->lower_bound(timed_data_.from());
    end_iterator_ = timed_data_.values()->end();
    count_ = std::distance(begin_iterator_, end_iterator_);
  }

  if (count_ > old_count)
    NotifyItemsAdded(old_count, count_ - old_count);
  else if (count_ < old_count)
    NotifyModelChanged();
}

void TimedDataModel::OnPropertyChanged(rt::TimedDataSpec& spec,
                                      const rt::PropertySet& properties) {
  if (properties.is_current_changed())
    Update();
}

void TimedDataModel::OnTimedDataCorrections(rt::TimedDataSpec& spec, size_t count,
                                            const scada::DataValue* tvqs) {
  assert(count > 0);
  Update();
}

void TimedDataModel::OnTimedDataReady(rt::TimedDataSpec& spec) {
  Update();
}

void TimedDataModel::OnTimedDataNodeModified(rt::TimedDataSpec& spec,
                                             const scada::PropertyIds& property_ids) {
  NotifyModelChanged();
}

void TimedDataModel::Iterate(int index) {
  const rt::TimedVQMap* values = timed_data_.values();
  assert(values);
  
  assert(index < count_);

  if (cached_index_ == -1) {
    cached_index_ = 0;
    cached_iterator_ = begin_iterator_;
  }

  if (index != cached_index_) {
    if (index > cached_index_) {
      if (index - cached_index_ > count_ - index) {
        cached_index_ = count_ - 1;
        cached_iterator_ = end_iterator_;
        --cached_iterator_;
      }
    } else {
      if (cached_index_ - index > index) {
        cached_index_ = 0;
        cached_iterator_ = begin_iterator_;
      }
    }
    while (index != cached_index_) {
      if (index > cached_index_) {
        assert(cached_iterator_ != end_iterator_);
        ++cached_iterator_;
        ++cached_index_;
      } else {
        assert(cached_iterator_ != begin_iterator_);
        --cached_iterator_;
        --cached_index_;
      }
    }
  }
}

scada::DataValue TimedDataModel::GetRowTVQ(int row) {
  if (row >= (int)timed_data_.values()->size())
    return timed_data_.current();
  
  Iterate(row);
  return scada::DataValue(cached_iterator_->second.vq.value,
                          cached_iterator_->second.vq.qualifier,
                          cached_iterator_->first,
                          cached_iterator_->second.collection_time);
}

int TimedDataModel::GetRowCount() {
  return count_;
}

void TimedDataModel::GetCell(ui::TableCell& cell) {
  scada::DataValue tvq = GetRowTVQ(cell.row);
  
  switch (cell.column_id) {
    case CID_TIME:
      cell.text = base::SysNativeMBToWide(FormatTime(tvq.source_timestamp,
          TIME_FORMAT_DATE | TIME_FORMAT_TIME | TIME_FORMAT_MSEC));
      break;
  
    case CID_VALUE:
      // Format without quality.
      cell.text = timed_data_.GetValueString(tvq.value, tvq.qualifier, 0);
      break;
  
    case CID_QUALITY:
      cell.text = base::SysNativeMBToWide(FormatQuality(tvq.qualifier));
      break;

    case CID_COLLECTION_TIME:
      cell.text = base::SysNativeMBToWide(FormatTime(tvq.server_timestamp,
          TIME_FORMAT_DATE | TIME_FORMAT_TIME | TIME_FORMAT_MSEC));
      break;
  }

  // highlight current time
  //	if (data && time == ((::Item*)data->rec)->val_time)
  //		cell.clrb = RGB(255, 255, 0);
}

void TimedDataModel::SetFormula(const std::string& formula) {
  cached_index_ = -1;

  rt::TimedDataSpec timed_data;
  try {
    timed_data.SetFrom(base::Time::Now() - base::TimeDelta::FromHours(1));
    timed_data.Connect(timed_data_service_, formula);
  } catch (const std::exception&) {
  }
  SetTimedData(timed_data);
}
