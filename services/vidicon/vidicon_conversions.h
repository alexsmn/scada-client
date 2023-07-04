#pragma once

#include "core/data_value.h"
#include "services/vidicon/vidicon_types.h"

#include <ATLComTime.h>

namespace vidicon {

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
  return timestamp.is_null()
             ? DATE{0}
             : static_cast<DATE>(COleDateTime{timestamp.ToFileTime()});
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

}  // namespace vidicon