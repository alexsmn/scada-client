#pragma once

#include "vidicon_types.h"

#include <functional>
#include <stop_token>
#include <string>

namespace vidicon {

using DataChangeHandler = std::function<void(const DataPointValue& value)>;

class DataPointManager {
 public:
  virtual ~DataPointManager() = default;

  virtual void Subscribe(const DataPointAddress& address,
                         std::stop_token cancelation,
                         const DataChangeHandler& handler) = 0;
};

}  // namespace vidicon
