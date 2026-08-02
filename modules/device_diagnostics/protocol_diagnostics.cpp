#include "device_diagnostics/protocol_diagnostics.h"

#include "model/devices_node_ids.h"

#include <array>

namespace {

// IEC 60870-5-104. These are the APCI state of the TCP connection, which is why
// they are read from the device's parent link rather than from the device.
//
// Availability and a 24-hour window are deliberately absent: both are history
// queries the model does not answer, and the screen was amended rather than the
// model stretched to meet it (ADR 0007). So is an error counter — the stack
// forms no definition of a link error, and InflightRetransmits is retries, not
// errors.
constexpr std::array<ProtocolField, 9> kIec60870LinkFields{{
    {scada::devices::id::Iec60870LinkType_LinkState, "LinkState", "State",
     ProtocolValueShape::kState},
    {scada::devices::id::Iec60870LinkType_LastConnected, "LastConnected",
     "Last connected", ProtocolValueShape::kTime},
    {scada::devices::id::Iec60870LinkType_RoundTripTime, "RoundTripTime",
     "Round-trip", ProtocolValueShape::kDuration},
    {scada::devices::id::Iec60870LinkType_SendSequenceNumber,
     "SendSequenceNumber", "Send seq V(S)"},
    {scada::devices::id::Iec60870LinkType_ReceiveSequenceNumber,
     "ReceiveSequenceNumber", "Recv seq V(R)"},
    {scada::devices::id::Iec60870LinkType_AcknowledgedSequenceNumber,
     "AcknowledgedSequenceNumber", "Acknowledged to"},
    {scada::devices::id::Iec60870LinkType_UnacknowledgedFrames,
     "UnacknowledgedFrames", "Unacknowledged frames"},
    {scada::devices::id::Iec60870LinkType_InflightRetransmits,
     "InflightRetransmits", "Retransmits in window"},
    {scada::devices::id::Iec60870LinkType_SendTimeoutExpired,
     "SendTimeoutExpired", "t1 timeout", ProtocolValueShape::kFlag},
}};

constexpr std::array<ProtocolDiagnostics, 1> kRegistry{{
    {"Iec60870DeviceType", "Iec60870LinkType", "Link (IEC 60870-5-104)",
     kIec60870LinkFields},
}};

}  // namespace

const ProtocolDiagnostics* ProtocolDiagnosticsFor(
    std::string_view device_type_browse_name) {
  if (device_type_browse_name.empty())
    return nullptr;
  for (const ProtocolDiagnostics& entry : kRegistry) {
    if (entry.device_type == device_type_browse_name)
      return &entry;
  }
  return nullptr;
}

std::string_view Iec60870LinkStateLabel(int state) {
  switch (state) {
    case 0x0002:
      return "Starting";
    case 0x0004:
      return "Running";
    case 0x0008:
      return "Test";
    case 0:
      return "Closed";
    default:
      return "Unknown";
  }
}
