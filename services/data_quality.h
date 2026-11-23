#pragma once

#include "aui/severity_colors.h"
#include "scada/data_value.h"
#include "scada/qualifier.h"

#include <optional>
#include <string>

// The one mapping from a live value's SCADA quality flags to the coarse
// good / uncertain / bad band every quality cue in the Qt client shows: the
// Inspector's quality pill, the table's Quality column and value tint, and the
// Explorer's status dot. Principle §5 (docs/client/ux/principles.md) asks for
// one palette applied identically on every surface; until backlog 1120 the
// three surfaces each had their own rule, so a stale value read «Недостоверно»
// in the Inspector and «Неопределённо» in the table, and the Explorer's amber
// dot meant "in alarm". Qt-free so it can be unit-tested without a
// QApplication.
//
// The bands follow the OPC-classic reading the flags are documented with in
// `core/scada/qualifier.h`:
//  - bad: BAD, FAILED, OFFLINE or MISCONFIGURED — no trustworthy value;
//  - uncertain: STALE alone — the last known value, too old to vouch for;
//  - good: everything else, including the Good_* caveats (manual, backup,
//    simulated, sporadic), which still carry a usable value.
// Bad dominates uncertain. Limit bits are not quality: an out-of-limit value
// can be perfectly good data. Alarm state is not quality either.
scada::aui::Quality DataQualityFor(scada::Qualifier qualifier);

// The same band for a whole value, with one thing the Qualifier alone cannot
// express: std::nullopt when nothing has ever been delivered. A
// default-constructed Qualifier is zero, and zero is not BAD, so mapping it
// through the Qualifier overload would report Good beside an empty readout.
// Prefer this overload wherever the DataValue is at hand.
std::optional<scada::aui::Quality> DataQualityFor(
    const scada::DataValue& value);

// The band's operator-facing label, translated: «Достоверно» /
// «Неопределённо» / «Недостоверно».
std::u16string DataQualityLabel(scada::aui::Quality quality);

// As above, with «Нет данных» for a value that was never delivered.
std::u16string DataQualityLabel(std::optional<scada::aui::Quality> quality);
