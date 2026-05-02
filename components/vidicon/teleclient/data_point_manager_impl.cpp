#include "vidicon/teleclient/data_point_manager_impl.h"

#include "base/executor.h"
#include "opc/opc_convertions.h"
#include "timed_data/timed_data_property.h"
#include "timed_data/timed_data_spec.h"
#include "vidicon/data_point_address.h"
#include "vidicon/vidicon_node_id.h"

namespace vidicon {

namespace {

struct DataPoint : public std::enable_shared_from_this<DataPoint> {
  DataPoint(std::stop_token cancelation, const DataChangeHandler& handler)
      : handler_{handler},
        stop_callback_{std::move(cancelation), [this] { Stop(); }} {
    timed_data_spec_.property_change_handler =
        [this](const PropertySet& properties) {
          if (properties.is_current_changed()) {
            handler_(
                opc::OpcDataValueConverter::ToOpc(timed_data_spec_.current()));
          }
        };
  }

  void Start(TimedDataService& service, const DataPointAddress& address) {
    self_ref_ = shared_from_this();

    const auto& node_id = ToNodeId(address);
    timed_data_spec_.Connect(service, node_id);

    if (const auto& current = timed_data_spec_.current(); !current.is_null()) {
      handler_(opc::OpcDataValueConverter::ToOpc(current));
    }
  }

  void Stop() { self_ref_.reset(); }

  const DataChangeHandler handler_;
  std::stop_callback<std::function<void()>> stop_callback_;

  TimedDataSpec timed_data_spec_;

  std::shared_ptr<DataPoint> self_ref_;
};

}  // namespace

// DataPointManagerImpl::Backend

struct DataPointManagerImpl::Backend {
  explicit Backend(TimedDataService& timed_data_service)
      : timed_data_service_{timed_data_service} {}

  void Subscribe(const DataPointAddress& address,
                 std::stop_token cancelation,
                 const DataChangeHandler& handler) {
    auto data_point =
        std::make_shared<DataPoint>(std::move(cancelation), handler);
    data_point->Start(timed_data_service_, address);
  }

  TimedDataService& timed_data_service_;
};

// DataPointManagerImpl

DataPointManagerImpl::DataPointManagerImpl(std::shared_ptr<Executor> executor,
                                           TimedDataService& timed_data_service)
    : executor_{std::move(executor)},
      backend_{std::make_unique<Backend>(timed_data_service)} {}

DataPointManagerImpl ::~DataPointManagerImpl() = default;

void DataPointManagerImpl::Subscribe(const DataPointAddress& address,
                                     std::stop_token cancelation,
                                     const DataChangeHandler& handler) {
  Dispatch(*executor_,
           [=, this] { backend_->Subscribe(address, cancelation, handler); });
}

}  // namespace vidicon
