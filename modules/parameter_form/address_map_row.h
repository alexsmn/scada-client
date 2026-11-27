#pragma once

#include <span>
#include <string>
#include <vector>

// One row of a device's address-map preview (config-workbench.html): a signal
// (information object) and its protocol addressing. Plain data with no Qt, so
// the host — which has the node service — can source these off the UI thread /
// in the Qt-free layer; DeviceParameterForm renders them read-only.
struct AddressMapRow {
  // Where the row comes from. A device can carry both: the items it polls and
  // the transmission rules it serves to a superior station.
  enum class Kind {
    // A transmission (retransmission) item: a source data item served at an
    // address of this device.
    kTransmission,
    // A data item whose channel (Input1 or its backup Input2) is an address
    // this device polls.
    kPolled,
  };

  std::u16string signal;   // source data-item display name.
  std::u16string type;     // signal-type tag (TS / TI / …); may be empty.
  std::u16string ioa;      // information-object / source address.
  std::u16string node_id;  // the source node's id.
  Kind kind = Kind::kTransmission;
};

// For each row of `rows`, whether another row of the same kind has the same
// address. Two retransmitted signals at one IOA silently corrupt what the
// superior station receives, and two polled signals at one address read the
// same value; the transmission rules view flags the first case, and the device
// form's address map now flags both (UX audit 2026-11-25, item 4.1). A polled
// row and a transmitted row at one address are two directions, not a
// conflict. Rows with no address never conflict.
std::vector<bool> FindDuplicateAddresses(std::span<const AddressMapRow> rows);
