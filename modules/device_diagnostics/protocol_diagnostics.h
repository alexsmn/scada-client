#pragma once

#include "scada/node_id.h"

#include <span>
#include <string_view>

// Which diagnostics a device gets, chosen by its protocol (ADR 0007).
//
// The diagnostics panel is reached by selecting ANY device, so it cannot assume
// IEC 60870: a Modbus device rendered through an IEC-104 panel would draw
// sequence-number rows that can never have values. An unregistered device type
// keeps the generic DeviceType counters and nothing more, so registering a
// protocol is additive and forgetting one costs depth rather than correctness.
//
// This mirrors the web client's `features/device-metrics/protocol-registry.ts`
// rather than sharing it — `client/` must stay standalone (see the client
// CLAUDE.md). The two must agree: the state and flag wordings below are what an
// operator reads to decide whether a link is healthy, and the parity rules make
// that class of meaning binding on both sides.
//
// Kept Qt-free so it is unit-testable without a QApplication, like
// device_link_state.h beside it.

// How a field's raw value is turned into text.
enum class ProtocolValueShape {
  kCount,     // plain number.
  kState,     // protocol state code -> word.
  kFlag,      // boolean -> condition, not "true"/"false".
  kDuration,  // milliseconds, with a unit.
  kTime,      // absolute instant.
};

// One row of a protocol's link section.
struct ProtocolField {
  // Resolves the instance child in the live app. The browse name is the
  // fallback the headless fixture needs, and the reason a differing namespace
  // index (ADR 0003) cannot break the lookup.
  scada::NodeId declaration_id;
  std::string_view browse_name;
  std::string_view label;  // English; translated at render time.
  ProtocolValueShape shape = ProtocolValueShape::kCount;
};

// An OPC UA Method on the link that the panel offers as a button.
//
// A Method, not a writable parameter: it has no value to hold, it acts on the
// live connection, and it is gated by the OPC UA Call permission
// (PermissionType.Call, Part 3 §8.55) rather than by the parameter form's Write
// gate. Those are different rights, and conflating them would let a session
// that may edit configuration issue control actions.
struct ProtocolLinkAction {
  // Declaration id of the Method on the link type. Resolved against the
  // instance the same way the fields are, so a differing namespace index
  // (ADR 0003) cannot break it.
  scada::NodeId method_id;
  std::string_view browse_name;
  std::string_view label;  // English; translated at render time.
  // Shown under the button when the session may not call it. A disabled
  // control must state a reason an operator can act on — "unimplemented" is
  // not one, and a dead button with no explanation is worse than no button.
  std::string_view denied_reason;
};

// What a registered protocol contributes to the panel.
struct ProtocolDiagnostics {
  // BrowseName of the device type this entry answers for.
  std::string_view device_type;
  // BrowseName of the link type its diagnostics live on. CHECKED, not assumed:
  // a device's parent is its link in the shipped model, but a flat model
  // parents devices straight onto the Devices folder, and reading link members
  // off a folder finds none — which would draw the whole section as em-dashes
  // instead of omitting it.
  std::string_view link_type;
  // Section heading. Names the protocol so an operator can see the rows
  // describe the LINK, not the device: several devices share one link, and two
  // of them will show the same numbers because it is the same link.
  std::string_view section_label;
  std::span<const ProtocolField> link_fields;
  // The protocol's link action, if it has one. `label` empty means none — a
  // protocol whose links cannot be commanded contributes no button rather than
  // a disabled one.
  ProtocolLinkAction link_action;
};

// The entry for a device type's BrowseName, or null when unregistered.
const ProtocolDiagnostics* ProtocolDiagnosticsFor(
    std::string_view device_type_browse_name);

// IEC 60870-5-104 link state (`Iec60870LinkType.LinkState`) as a word.
// Mirrors Iec104Transport's IEC_START / IEC_RUN / IEC_TEST; the nodeset
// documents the values on the node itself, which is the contract both clients
// read. An unrecognised code is named as unknown rather than guessed at.
std::string_view Iec60870LinkStateLabel(int state);
