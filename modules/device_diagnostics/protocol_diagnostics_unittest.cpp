#include "device_diagnostics/protocol_diagnostics.h"

#include "model/devices_node_ids.h"

#include <gtest/gtest.h>

#include <algorithm>

namespace {

// The dispatch exists because the diagnostics panel is reached by selecting ANY
// device. These mirror the web client's protocol-registry tests deliberately:
// the two registries are separate implementations (client/ must stay
// standalone) of one contract, and an operator reading either must be told the
// same thing about the same link.

TEST(ProtocolDiagnosticsTest, AnswersForIec60870) {
  const ProtocolDiagnostics* entry =
      ProtocolDiagnosticsFor("Iec60870DeviceType");
  ASSERT_TRUE(entry);
  EXPECT_EQ(entry->link_type, "Iec60870LinkType");
  EXPECT_FALSE(entry->link_fields.empty());
}

// The failure this prevents: a Modbus device drawn through an IEC-104 panel
// shows sequence-number rows that can never have values.
TEST(ProtocolDiagnosticsTest, AnswersForNoOtherProtocol) {
  EXPECT_FALSE(ProtocolDiagnosticsFor("ModbusDeviceType"));
  EXPECT_FALSE(ProtocolDiagnosticsFor("Iec61850DeviceType"));
  EXPECT_FALSE(ProtocolDiagnosticsFor(""));
  EXPECT_FALSE(ProtocolDiagnosticsFor("DeviceType"));
}

// Every field must carry both a declaration id and a browse name. The
// declaration resolves it in the live app; the browse name is the fallback, and
// is what keeps the lookup working when a tier serves the SCADA namespace at a
// different index (ADR 0003).
TEST(ProtocolDiagnosticsTest, EveryFieldCarriesBothWaysToResolveIt) {
  const ProtocolDiagnostics* entry =
      ProtocolDiagnosticsFor("Iec60870DeviceType");
  ASSERT_TRUE(entry);
  for (const ProtocolField& field : entry->link_fields) {
    EXPECT_FALSE(field.declaration_id.is_null()) << field.label;
    EXPECT_FALSE(field.browse_name.empty()) << field.label;
    EXPECT_FALSE(field.label.empty());
  }
}

// The rows that are NOT here are as deliberate as the ones that are:
// availability and a 24-hour window are history queries the model does not
// answer, and there is no error counter — retransmits are retries, not errors.
TEST(ProtocolDiagnosticsTest, DoesNotDeclareRowsTheModelCannotBack) {
  const ProtocolDiagnostics* entry =
      ProtocolDiagnosticsFor("Iec60870DeviceType");
  ASSERT_TRUE(entry);
  const auto has = [entry](std::string_view browse_name) {
    return std::ranges::any_of(entry->link_fields,
                               [browse_name](const ProtocolField& field) {
                                 return field.browse_name == browse_name;
                               });
  };
  EXPECT_FALSE(has("Availability"));
  EXPECT_FALSE(has("Errors"));
  EXPECT_TRUE(has("InflightRetransmits"));
}

// Shapes drive rendering: a state must not print as a bare number and a t1 flag
// must not print as "true", or an operator has to remember which value means
// trouble.
TEST(ProtocolDiagnosticsTest, StateAndFlagFieldsDeclareTheirShape) {
  const ProtocolDiagnostics* entry =
      ProtocolDiagnosticsFor("Iec60870DeviceType");
  ASSERT_TRUE(entry);
  const auto shape_of = [entry](std::string_view browse_name) {
    auto it = std::ranges::find_if(entry->link_fields,
                                   [browse_name](const ProtocolField& field) {
                                     return field.browse_name == browse_name;
                                   });
    return it->shape;
  };
  EXPECT_EQ(shape_of("LinkState"), ProtocolValueShape::kState);
  EXPECT_EQ(shape_of("SendTimeoutExpired"), ProtocolValueShape::kFlag);
  EXPECT_EQ(shape_of("RoundTripTime"), ProtocolValueShape::kDuration);
  EXPECT_EQ(shape_of("LastConnected"), ProtocolValueShape::kTime);
  EXPECT_EQ(shape_of("SendSequenceNumber"), ProtocolValueShape::kCount);
}

// "Reconnect now" from the mockup. It is a Method ON THE LINK — the APCI and
// the TCP connection belong to the link, and several devices share one, so a
// per-device Reconnect would let any of them drop the link out from under its
// siblings.
TEST(ProtocolDiagnosticsTest, Iec60870DeclaresTheReconnectLinkAction) {
  const ProtocolDiagnostics* entry =
      ProtocolDiagnosticsFor("Iec60870DeviceType");
  ASSERT_TRUE(entry);
  EXPECT_EQ(entry->link_action.method_id,
            scada::NodeId{scada::devices::id::Iec60870LinkType_Reconnect});
  EXPECT_EQ(entry->link_action.browse_name, "Reconnect");
  EXPECT_FALSE(entry->link_action.label.empty());
}

// A disabled control must state a reason an operator can act on, so an action
// that can be refused must carry one. An entry with a label and no reason
// would render a dead button with nothing to explain it — the exact failure
// this rule exists to prevent.
TEST(ProtocolDiagnosticsTest, EveryLinkActionCarriesADeniedReason) {
  const ProtocolDiagnostics* entry =
      ProtocolDiagnosticsFor("Iec60870DeviceType");
  ASSERT_TRUE(entry);
  if (entry->link_action.label.empty())
    return;  // no action declared; nothing to explain.
  EXPECT_FALSE(entry->link_action.denied_reason.empty());
  EXPECT_FALSE(entry->link_action.method_id.is_null());
}

TEST(ProtocolDiagnosticsTest, LinkStateLabelsMatchTheTransportConstants) {
  EXPECT_EQ(Iec60870LinkStateLabel(0), "Closed");
  EXPECT_EQ(Iec60870LinkStateLabel(0x0002), "Starting");
  EXPECT_EQ(Iec60870LinkStateLabel(0x0004), "Running");
  EXPECT_EQ(Iec60870LinkStateLabel(0x0008), "Test");
}

// Naming an unrecognised code rather than guessing: a future transport state
// must not silently render as one of the known ones.
TEST(ProtocolDiagnosticsTest, UnknownLinkStateIsNamedNotGuessed) {
  EXPECT_EQ(Iec60870LinkStateLabel(99), "Unknown");
  EXPECT_EQ(Iec60870LinkStateLabel(-1), "Unknown");
}

}  // namespace
