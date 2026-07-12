#pragma once

#include <functional>
#include <stop_token>

#include "vidicon/vidicon_compat.h"

namespace opc_client {
struct DataValue;
}

namespace scada::vidicon {
struct DataPointAddress;
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
