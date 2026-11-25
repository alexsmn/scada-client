#pragma once

#include "base/any_executor.h"
#include "base/awaitable.h"
#include "modules/transmission/transmission_devices.h"

#include <functional>
#include <vector>

namespace scada::aui {
class GridModel;
}

class QWidget;

// Inputs for the Transmission view's destination rail (Qt): the mockup's left
// rail listing every transmission-capable device with its rule count, driving
// which device's rules the grid shows.
//
// The rail is decoupled from the view behind callbacks so the widget is
// testable: `browse` enumerates the devices asynchronously
// (BrowseTransmissionDevices in production), `current_count` supplies the
// open device's live rule count, `current_conflicts` (optional) how many of
// its rules share a destination address with another, and `model` (optional)
// provides the grid's change notifications that keep both current.
struct TransmissionDestinationRailContext {
  AnyExecutor executor;
  std::function<Awaitable<std::vector<TransmissionDeviceEntry>>()> browse;
  // The device whose rules the grid currently shows; preselected without
  // firing `on_device`.
  scada::NodeId current;
  std::function<int()> current_count;
  // Rules on the open device whose address another rule also carries
  // (`TransmissionModel::CountDuplicateAddresses`). Non-zero, the row gains
  // a warning mark and the count, named on hover (backlog 1128). Only the
  // open device's is known: the other devices' addresses are not fetched.
  std::function<int()> current_conflicts;
  scada::aui::GridModel* model = nullptr;
  std::function<void(const scada::NodeId&)> on_device;
};

// Builds the rail. The returned widget owns its rows and invokes `on_device`
// when the operator picks another device.
QWidget* MakeTransmissionDestinationRail(
    TransmissionDestinationRailContext context);
