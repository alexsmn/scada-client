#include "services/vidicon/data_point_manager_impl.h"

#include "base/executor.h"
#include "core/data_value.h"
#include "core/monitored_item.h"
#include "services/vidicon/data_point_manager.h"
#include "timed_data/timed_data_spec.h"

#include <ATLComTime.h>
#include <unordered_map>
#include <vector>

namespace vidicon {

namespace {

inline CComVariant ToComVariant(const scada::Variant& value) {
  switch (value.type()) {
    case scada::Variant::EMPTY:
      return {};
    case scada::Variant::BOOL:
      return value.as_bool();
    case scada::Variant::INT8:
      return value.get<scada::Int8>();
    case scada::Variant::UINT8:
      return value.get<scada::UInt8>();
    case scada::Variant::INT16:
      return value.get<scada::Int16>();
    case scada::Variant::UINT16:
      return value.get<scada::UInt16>();
    case scada::Variant::INT32:
      return value.get<scada::Int32>();
    case scada::Variant::UINT32:
      return value.get<scada::UInt32>();
    case scada::Variant::DOUBLE:
      return value.get<scada::Double>();
    default:
      assert(false);
      return {};
  }
}

inline DATE ToDATE(const scada::DateTime& timestamp) {
  return COleDateTime{timestamp.ToFileTime()};
}

inline unsigned ToOpcQuality(scada::Qualifier qualifier) {
  // TODO: Quality.
  return 0xC0;  // OPC_QUALITY_GOOD
}

inline DataPointValue ToDataPointValue(const scada::DataValue& data_value) {
  return {.value = ToComVariant(data_value.value),
          .time = ToDATE(data_value.source_timestamp),
          .quality = ToOpcQuality(data_value.qualifier)};
}

struct DataPoint : public std::enable_shared_from_this<DataPoint> {
  DataPoint(const TimedDataSpec& timed_data_spec,
            std::stop_token cancelation,
            const DataChangeHandler& handler)
      : timed_data_spec_{timed_data_spec},
        stop_callback_{std::move(cancelation), [this] { Stop(); }} {
    timed_data_spec_.property_change_handler =
        [this, handler](const PropertySet& properties) {
          if (properties.is_current_changed()) {
            handler(ToDataPointValue(timed_data_spec_.current()));
          }
        };
  }

  void Start() { self_ref_ = shared_from_this(); }
  void Stop() { self_ref_.reset(); }

  TimedDataSpec timed_data_spec_;
  std::stop_token cancelation_;
  std::stop_callback<std::function<void()>> stop_callback_;

  std::shared_ptr<DataPoint> self_ref_;
};

}  // namespace

// DataPointManagerImpl::Backend

struct DataPointManagerImpl::Backend {
  explicit Backend(TimedDataService& timed_data_service)
      : timed_data_service_{timed_data_service} {}

  void Subscribe(const std::string& formula,
                 std::stop_token cancelation,
                 const DataChangeHandler& handler) {
    TimedDataSpec timed_data_spec{timed_data_service_, formula};
    auto data_point = std::make_shared<DataPoint>(
        timed_data_spec, std::move(cancelation), handler);
    data_point->Start();
  }

  TimedDataService& timed_data_service_;
};

// DataPointManagerImpl

DataPointManagerImpl::DataPointManagerImpl(std::shared_ptr<Executor> executor,
                                           TimedDataService& timed_data_service)
    : executor_{std::move(executor)},
      backend_{std::make_unique<Backend>(timed_data_service)} {}

DataPointManagerImpl ::~DataPointManagerImpl() = default;

void DataPointManagerImpl::Subscribe(const std::string& formula,
                                     std::stop_token cancelation,
                                     const DataChangeHandler& handler) {
  Dispatch(*executor_,
           [=, this] { backend_->Subscribe(formula, cancelation, handler); });
}

}  // namespace vidicon
