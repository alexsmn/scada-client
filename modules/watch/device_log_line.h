#pragma once

#include <string_view>

// Direction of a device log line, recovered from the marker the device drivers
// put at the front of the message.
//
// The drivers tag protocol traffic as it is written: `#` for something received
// from the device, `$` for something sent to it (see
// `scada-tier-iec104/modules/iec60870/lib/*`, e.g. `"#RX: {} ({} bytes)"` for a
// raw frame and `"$TX: Send write confirmation [...]"` for a command). Plain
// operational lines — "Transmission completed", link warnings — carry no
// marker.
//
// The marker survives to the client untouched: DeviceLogger passes the message
// straight into the DeviceWatchEventType event
// (`scada-server-framework/devices/module/device_logger.cpp`), and nothing
// strips it on the way. That is what makes a frame-trace mode possible without
// a server change.
enum class DeviceLogDirection {
  // No marker: an ordinary log line, not protocol traffic.
  kNone,
  // `#` — received from the device.
  kInbound,
  // `$` — sent to the device.
  kOutbound,
};

// A device log message split into its direction and the text an operator should
// read (the marker itself is chrome and is not shown).
struct DeviceLogLine {
  DeviceLogDirection direction = DeviceLogDirection::kNone;
  std::u16string_view text;
};

// Classifies `message`. Unmarked messages are returned verbatim, so this is
// safe to run over every line including drivers that do not tag anything.
DeviceLogLine ClassifyDeviceLogLine(std::u16string_view message);

// Whether `line` is protocol traffic — i.e. whether the frame-trace mode shows
// it. Ordinary log lines are excluded, which is the whole point of the mode:
// on a busy link the traffic is the signal and the status lines are noise.
inline bool IsProtocolTraffic(const DeviceLogLine& line) {
  return line.direction != DeviceLogDirection::kNone;
}
