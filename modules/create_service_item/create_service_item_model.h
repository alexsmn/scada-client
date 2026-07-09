#pragma once

#include "base/lifetime.h"
#include "modules/create_service_item/create_service_item_dialog.h"
#include "ui/common/client_utils.h"

class CreateServiceItemModel : private CreateServiceItemContext {
 public:
  explicit CreateServiceItemModel(CreateServiceItemContext&& context);

  const NamedNodes& devices() const SCADA_LIFETIME_BOUND { return devices_; }
  const NamedNodes& components() const SCADA_LIFETIME_BOUND {
    return components_;
  }

  int device_index() const { return device_index_; }
  void SetDeviceIndex(int index);

  struct RunParams {
    std::vector<int> component_indexes;
  };

  void Run(const RunParams& params);

 private:
  NamedNodes devices_;
  NamedNodes components_;

  int device_index_ = -1;
};
