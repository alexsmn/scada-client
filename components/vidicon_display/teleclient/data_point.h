
#pragma once

#include "core/data_value.h"
#include "core/monitored_item.h"
#include "timed_data/timed_data_spec.h"

#include <ATLComTime.h>
#include <atlcomcli.h>
#include <boost/signals2.hpp>
#include <memory>
#include <mutex>
#include <vector>

struct DataPointValue {
  CComVariant value;
  DATE time = 0;
  unsigned quality = 0;
};

class DataPoint {
 public:
  virtual ~DataPoint() = default;

  virtual CComVariant GetValue() const = 0;
  virtual DATE GetTime() const = 0;
  virtual unsigned GetQuality() const = 0;

  using DataChangedSignal = boost::signals2::signal<void()>;
  mutable DataChangedSignal data_changed_signal;
};

class DataPointImpl : public DataPoint,
                      public std::enable_shared_from_this<DataPointImpl> {
 public:
  explicit DataPointImpl(std::wstring formula);

  void Init();

  // DataPoint
  virtual CComVariant GetValue() const override;
  virtual DATE GetTime() const override;
  virtual unsigned GetQuality() const override;

 private:
  const std::wstring formula_;

  mutable std::mutex mutex_;
  DataPointValue data_value_;
};

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
  return 0xC0;  // OPC_QUALITY_GOOD
}

inline DataPointValue ToDataPointValue(const scada::DataValue& data_point) {
  return {
      ToComVariant(data_point.value),
      ToDATE(data_point.server_timestamp),
      ToOpcQuality(data_point.qualifier),
  };
}

}  // namespace

DataPointImpl::DataPointImpl(std::wstring formula) : formula_{formula} {}

void DataPointImpl::Init() {
  /*monitored_item_->set_data_change_handler(
      [this, ref = shared_from_this()](const scada::DataValue& data_value) {
        {
          std::lock_guard lock{mutex_};
          data_value_ = ToDataPointValue(data_value);
        }

        data_changed_signal();
      });

  monitored_item_->Subscribe();*/
}

inline CComVariant DataPointImpl::GetValue() const {
  std::lock_guard lock{mutex_};
  return data_value_.value;
}

inline DATE DataPointImpl::GetTime() const {
  std::lock_guard lock{mutex_};
  return data_value_.time;
}

inline unsigned DataPointImpl::GetQuality() const {
  std::lock_guard lock{mutex_};
  return data_value_.quality;
}