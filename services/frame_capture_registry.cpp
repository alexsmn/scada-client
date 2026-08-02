#include "services/frame_capture_registry.h"

#include <algorithm>

void FrameCaptureRegistry::SetArmed(const scada::NodeId& device_id,
                                    std::u16string display_name,
                                    bool armed) {
  const auto found = std::ranges::find(armed_, device_id,
                                       &ArmedDevice::device_id);

  if (armed) {
    if (found != armed_.end()) {
      // Already listed; only the name can have changed (the device was
      // renamed, or was unnamed when first armed).
      if (found->display_name == display_name)
        return;
      found->display_name = std::move(display_name);
    } else {
      armed_.push_back({device_id, std::move(display_name)});
    }
  } else {
    if (found == armed_.end())
      return;
    armed_.erase(found);
  }

  changed_signal_();
}

bool FrameCaptureRegistry::IsArmed(const scada::NodeId& device_id) const {
  return std::ranges::find(armed_, device_id, &ArmedDevice::device_id) !=
         armed_.end();
}
