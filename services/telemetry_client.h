#pragma once

#include "services/telemetry.h"

namespace boost::asio {
class io_context;
}

class TelemetryClient {
 public:
  void Init();

  void Send(const TelemetryEvent& event);

 private:
  boost::asio::io_context& ioc_;
};
