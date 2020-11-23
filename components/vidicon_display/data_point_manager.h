#pragma once

#include <memory>
#include <string_view>

class DataPoint;

class DataPointManager {
 public:
  std::shared_ptr<DataPoint> GetDataPoint(std::wstring_view address) {
    return std::make_shared<DataPoint>();
  }
};