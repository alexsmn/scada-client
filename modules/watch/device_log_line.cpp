#include "modules/watch/device_log_line.h"

namespace {

// The markers the device drivers write. Deliberately matched as a bare leading
// character rather than the whole `#RX:` / `$TX:` token: the direction word is
// not consistent across drivers (an IEC 60870 server logs `#RX: Server write
// request`, the connection layer logs `#RX: <hex>`), while the marker itself
// is, and it is the marker that carries the meaning.
constexpr char16_t kInboundMarker = u'#';
constexpr char16_t kOutboundMarker = u'$';

}  // namespace

DeviceLogLine ClassifyDeviceLogLine(std::u16string_view message) {
  if (message.empty())
    return {};

  DeviceLogDirection direction = DeviceLogDirection::kNone;
  switch (message.front()) {
    case kInboundMarker:
      direction = DeviceLogDirection::kInbound;
      break;
    case kOutboundMarker:
      direction = DeviceLogDirection::kOutbound;
      break;
    default:
      // Unmarked: an ordinary log line. Return it whole.
      return {.direction = DeviceLogDirection::kNone, .text = message};
  }

  return {.direction = direction, .text = message.substr(1)};
}
