#pragma once

#include "services/vidicon/data_point_address.h"

#include <functional>
#include <stop_token>

namespace opc_client {
struct DataValue;
}

namespace vidicon {

using DataChangeHandler =
    std::function<void(const opc_client::DataValue& data_value)>;

class DataPointManager {
 public:
  virtual ~DataPointManager() = default;

  virtual void Subscribe(const DataPointAddress& address,
                         std::stop_token cancelation,
                         const DataChangeHandler& handler) = 0;
};

}  // namespace vidicon
