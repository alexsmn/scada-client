#pragma once

#include "base/lifetime.h"
#include "scada/node_id.h"

#include <boost/signals2/connection.hpp>
#include <boost/signals2/signal.hpp>
#include <functional>
#include <span>
#include <string>
#include <vector>

// The devices this client has armed for protocol-frame capture.
//
// Arming is server-side state — a write to the device's FrameCapture variable
// — but the client keeps its own list so the status strip can say what *this*
// operator left running. That is the whole point of the indicator: an armed
// capture makes a busy link raise an event per frame, and the person who armed
// it is the one who has to remember to stop it.
//
// Deliberately not a subscription to the server's variables: the status strip
// would then have to subscribe to every device in the address space to notice
// one armed elsewhere, and what it needs to report is this client's own doing.
class FrameCaptureRegistry {
 public:
  struct ArmedDevice {
    scada::NodeId device_id;
    std::u16string display_name;
  };

  using ChangeCallback = std::function<void()>;

  // Adds or removes `device_id`. Idempotent, so a view may disarm on both the
  // mode switch and its own teardown without double-counting.
  void SetArmed(const scada::NodeId& device_id,
                std::u16string display_name,
                bool armed);

  bool IsArmed(const scada::NodeId& device_id) const;

  // In arming order, so the strip names the oldest forgotten capture first.
  std::span<const ArmedDevice> armed() const SCADA_LIFETIME_BOUND {
    return armed_;
  }

  [[nodiscard]] boost::signals2::scoped_connection SubscribeChanged(
      const ChangeCallback& callback) {
    return changed_signal_.connect(callback);
  }

 private:
  std::vector<ArmedDevice> armed_;

  boost::signals2::signal<void()> changed_signal_;
};
