#pragma once

#include <functional>
#include <stop_token>
#include <string>

namespace vidicon {

struct DataPointValue;

using DataChangeHandler = std::function<void(const DataPointValue& value)>;

class DataPointManager {
 public:
  virtual ~DataPointManager() = default;

  virtual void Subscribe(const std::string& formula,
                         std::stop_token cancelation,
                         const DataChangeHandler& handler) = 0;
};

}  // namespace vidicon
