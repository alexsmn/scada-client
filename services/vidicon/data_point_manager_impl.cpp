#include "services/vidicon/data_point_manager_impl.h"

#include "base/executor.h"
#include "model/namespaces.h"
#include "model/node_id_util.h"
#include "model/opc_node_ids.h"
#include "opc/opc_convertions.h"
#include "services/vidicon/data_point_address.h"
#include "timed_data/timed_data_property.h"
#include "timed_data/timed_data_spec.h"

#include <boost/locale/encoding_utf.hpp>

namespace vidicon {

namespace {

scada::NodeId MakeAddressNodeId(const DataPointAddress& address) {
  if (address.vidicon_id != 0) {
    return scada::NodeId{static_cast<scada::NumericId>(address.vidicon_id),
                         NamespaceIndexes::VIDICON};
  } else {
    return MakeNestedNodeId(opc::id::OPC, boost::locale::conv::utf_to_utf<char>(
                                              address.opc_address));
  }
}

struct DataPoint : public std::enable_shared_from_this<DataPoint> {
  DataPoint(std::stop_token cancelation, const DataChangeHandler& handler)
      : handler_{handler},
        stop_callback_{std::move(cancelation), [this] { Stop(); }} {
    timed_data_spec_.property_change_handler =
        [this](const PropertySet& properties) {
          if (properties.is_current_changed()) {
            handler_(opc::OpcDataValueConverter::Convert(
                timed_data_spec_.current()));
          }
        };
  }

  void Start(TimedDataService& service, const DataPointAddress& address) {
    self_ref_ = shared_from_this();

    auto node_id = MakeAddressNodeId(address);
    timed_data_spec_.Connect(service, node_id);

    if (const auto& current = timed_data_spec_.current(); !current.is_null()) {
      handler_(opc::OpcDataValueConverter::Convert(current));
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
