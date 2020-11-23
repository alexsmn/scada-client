
#pragma once

#include <memory>
#include <string_view>

class DataPoint;

class DataPointManager {
 public:
  virtual ~DataPointManager() = default;

  virtual std::shared_ptr<DataPoint> GetDataPoint(
      std::wstring_view formula) = 0;
};

class DataPointManagerImpl : public DataPointManager {
 public:
  virtual std::shared_ptr<DataPoint> GetDataPoint(
      std::wstring_view formula) override {
    return std::make_shared<DataPointImpl>(std::wstring{formula});
  }
};
