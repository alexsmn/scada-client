#pragma once

// The connection band for a device's diagnostics hero, mirroring the
// link-status hero in client/docs/ui-mockups/screens/config-workbench.html. A
// pure mapping so it can be unit-tested without a running QApplication.
enum class DeviceLinkBand {
  kUp,        // enabled and online — the link is up (good).
  kDown,      // enabled but not online — the link is down (bad).
  kDisabled,  // not enabled/polled — neither up nor down, so it reads neutral
              // rather than alarming.
};

// Maps a device's runtime {enabled, online} flags to the hero band. A disabled
// device is deliberately neither up nor down: it is simply not polled, so it
// must not render as an alarm.
DeviceLinkBand DeviceLinkBandFor(bool enabled, bool online);
