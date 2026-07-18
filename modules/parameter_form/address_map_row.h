#pragma once

#include <string>

// One row of a device's address-map preview (config-workbench.html): a signal
// (information object) and its protocol addressing. Plain data with no Qt, so
// the host — which has the node service — can source these off the UI thread /
// in the Qt-free layer; DeviceParameterForm renders them read-only.
struct AddressMapRow {
  std::u16string signal;   // source data-item display name.
  std::u16string type;     // signal-type tag (TS / TI / …); may be empty.
  std::u16string ioa;      // information-object / source address.
  std::u16string node_id;  // the source node's id.
};
