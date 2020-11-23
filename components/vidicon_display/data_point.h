#pragma once

#include <ATLComTime.h>
#include <atlcomcli.h>
#include <boost/signals2.hpp>
#include <memory>
#include <vector>

class DataPoint {
 public:
  virtual ~DataPoint() = default;

  virtual CComVariant GetValue();
  virtual DATE GetTime();
  virtual unsigned GetQuality();

  using DataChangedSignal = boost::signals2::signal<void()>;
  DataChangedSignal data_changed_signal;
};

inline CComVariant DataPoint::GetValue() {
  return {};
}

inline DATE DataPoint::GetTime() {
  return COleDateTime::GetCurrentTime();
}

inline unsigned DataPoint::GetQuality() {
  return 0xC0;  // OPC_QUALITY_GOOD
}