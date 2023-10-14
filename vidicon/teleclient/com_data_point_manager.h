#pragma once

#include "vidicon/teleclient/com_data_point_impl.h"
#include "vidicon/teleclient/data_point_address.h"

#include <atlbase.h>

#include <atlcom.h>
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

  Microsoft::WRL::ComPtr<IDataPoint> GetComDataPoint(
      std::wstring_view address) {
    auto parsed_address = ParseDataPointAddress(address);
    if (!parsed_address.has_value()) {
      return nullptr;
    }

    std::lock_guard lock{mutex_};

    auto i = com_data_points_.find(*parsed_address);
    if (i != com_data_points_.end()) {
      return i->second;
    }

    auto* com_data_point = new CComObjectNoLock<ComDataPointImpl>();
    com_data_points_.try_emplace(*parsed_address, com_data_point);

    com_data_point->Init(
        data_point_manager_, *parsed_address,
        [this, address = *parsed_address] { com_data_points_.erase(address); });

    return com_data_point;
  }

 private:
  DataPointManager& data_point_manager_;

  std::mutex mutex_;
  std::unordered_map<DataPointAddress, Microsoft::WRL::ComPtr<IDataPoint>>
      com_data_points_;
};

}  // namespace vidicon