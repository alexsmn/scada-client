#pragma once

#include <string>

// One row of a device's limits preview (config-workbench.html's Limits tab): an
// analog signal and its four warning/alarm bands. Plain data with no Qt, so the
// host — which has the node service — sources these off the Qt layer;
// DeviceParameterForm renders them read-only. An unset band is an empty string.
struct LimitRow {
  std::u16string signal;  // analog data-item display name.
  std::u16string lolo;    // LoLo (lower alarm) limit.
  std::u16string lo;      // Lo (lower warning) limit.
  std::u16string hi;      // Hi (upper warning) limit.
  std::u16string hihi;    // HiHi (upper alarm) limit.
};
