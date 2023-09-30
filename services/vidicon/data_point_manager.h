#pragma once

#include "opc/opc_types.h"
#include "services/vidicon/data_point_address.h"

#include <functional>
#include <stop_token>

namespace vidicon {

using DataChangeHandler =
    std::function<void(const opc::OpcDataValue& data_value)>;

class DataPointManager {
 public:
  virtual ~DataPointManager() = default;

  virtual void Subscribe(const DataPointAddress& address,
                         std::stop_token cancelation,
                         const DataChangeHandler& handler) = 0;
};

}  // namespace vidicon
