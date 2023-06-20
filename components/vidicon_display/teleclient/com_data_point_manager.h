#pragma once

#include "components/vidicon_display/teleclient/com_data_point_impl.h"

#include <functional>
#include <mutex>
#include <string_view>
#include <unordered_map>

class DataPoint;

using DataPointProvider =
    std::function<std::shared_ptr<DataPoint>(std::wstring_view address)>;

class ComDataPointManager {
 public:
  explicit ComDataPointManager(DataPointProvider data_point_provider)
      : data_point_provider_{std::move(data_point_provider)} {}

  CComPtr<TeleClientLib::IDataPoint> GetComDataPoint(
      std::wstring_view address) {
    auto data_point = data_point_provider_(address);
    if (!data_point)
      return nullptr;

    std::lock_guard lock{mutex_};

    auto i = com_data_points_.find(data_point);
    if (i != com_data_points_.end())
      return i->second;

    auto* com_data_point = new CComObjectNoLock<ComDataPointImpl>();
    com_data_point->Init(
        data_point, [this, data_point] { com_data_points_.erase(data_point); });
    com_data_points_.emplace(data_point, com_data_point);
    return com_data_point;
  }

  void RemoveComDataPoint(TeleClientLib::IDataPoint& data_point) {}

 private:
  const DataPointProvider data_point_provider_;

  std::mutex mutex_;
  std::unordered_map<std::shared_ptr<DataPoint>,
                     CComPtr<TeleClientLib::IDataPoint>>
      com_data_points_;
};