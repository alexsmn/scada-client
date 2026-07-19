#pragma once

#include "scada/basic_types.h"

#include <string>

namespace scada {
class NodeId;
}

// Pure, node-service-free helpers backing the reshell transmission-rules
// editor (client/docs/ui-mockups/screens/transmission-rules.html). They map the
// bits of a transmission rule the client node model actually exposes — the
// protocol (from the item's type), the source signal kind, and a compact
// source-to-address summary — into display strings. Unit-testable without Qt or
// a node service.

// Human-readable protocol label for a transmission-item type definition:
// "Modbus", "IEC 60870-5-104", "IEC 61850", or "" for a plain/unknown item.
std::u16string TransmissionProtocolLabel(const scada::NodeId& type_definition_id);

// The signal-type tag (TS for a discrete source, TI for an analog source) for a
// source data-item type definition; empty for types without a simple tag.
std::u16string TransmissionSignalTag(const scada::NodeId& source_type_id);

// The compact rule summary shown in the inspector header, e.g. "Ua → 2001".
// An empty source name renders as an em dash so the arrow still reads.
std::u16string TransmissionRuleSummary(const std::u16string& source_name,
                                       scada::Int32 ioa);
