#pragma once

#include "aui/severity_colors.h"
#include "services/device_state_notifier.h"

#include <optional>

class NodeRef;

// The hardware-tree status-dot quality for a device's connection state — the
// dots in docs/product/ui-mockups/screens/config-workbench.html's Hardware
// explorer. Online reads good, Offline reads bad, Disabled reads uncertain
// (present but not polled, not an alarm), and Unknown gets no dot. Pure, so it
// is unit-testable without Qt; the caller resolves the colour through
// scada::aui::QualityColor (which is itself theme-gated).
std::optional<scada::aui::Quality> DeviceStateQuality(DeviceState state);

// Computes a device's coarse DeviceState from its current Online/Disabled Value
// attributes, read directly from `device` (no delivered monitored-item value
// required). This is the immediate/offscreen fallback for the hardware-tree dot
// when the live DeviceStateNotifier has not resolved a state yet — right after
// the tree loads, and in the headless capture. Mirrors
// DeviceStateNotifier::CalculateDeviceState. Returns Unknown when the device
// exposes no Online variable.
DeviceState DeviceStateFromNode(const NodeRef& device);
