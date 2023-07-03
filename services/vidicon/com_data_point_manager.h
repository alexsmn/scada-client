#pragma once

#include "services/vidicon/com_data_point_impl.h"

#include <functional>
#include <mutex>
#include <string_view>
#include <unordered_map>
#include <wrl/client.h>

namespace vidicon {

class ComDataPointManager {
 public:
  explicit ComDataPointManager(DataPointManager& data_point_manager)
      : data_point_manager_{data_point_manager} {}

  Microsoft::WRL::ComPtr<TeleClientLib::IDataPoint> GetComDataPoint(
      const std::wstring& address) {
    std::lock_guard lock{mutex_};

    auto i = com_data_points_.find(address);
    if (i != com_data_points_.end()) {
      return i->second;
    }

    auto* com_data_point = new CComObjectNoLock<ComDataPointImpl>();
    com_data_points_.try_emplace(address, com_data_point);

    com_data_point->Init(data_point_manager_, address,
                         [this, address] { com_data_points_.erase(address); });

    return com_data_point;
  }

 private:
  DataPointManager& data_point_manager_;

  std::mutex mutex_;
  std::unordered_map<std::wstring,
                     Microsoft::WRL::ComPtr<TeleClientLib::IDataPoint>>
      com_data_points_;
};

}  // namespace vidicon