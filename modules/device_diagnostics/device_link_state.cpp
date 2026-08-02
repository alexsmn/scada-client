#include "device_diagnostics/device_link_state.h"

DeviceLinkBand DeviceLinkBandFor(bool enabled, bool online) {
  if (!enabled)
    return DeviceLinkBand::kDisabled;
  return online ? DeviceLinkBand::kUp : DeviceLinkBand::kDown;
}
