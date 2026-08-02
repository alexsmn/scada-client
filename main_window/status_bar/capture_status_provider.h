#pragma once

#include "aui/color.h"
#include "services/frame_capture_registry.h"

#include <boost/signals2/connection.hpp>
#include <functional>
#include <optional>
#include <string>

// The status-strip cell for an armed frame capture ("Capturing · КП-02").
//
// Empty while nothing is armed, so the strip is unchanged in normal operation.
// Armed, it is coloured with the bad-quality token — not because a capture is
// an alarm, but because it is a state the operator has to notice and undo, and
// that is the one colour the strip already uses to mean "look at me".
class CaptureStatusProvider final {
 public:
  using ChangeNotifier = std::function<void()>;

  explicit CaptureStatusProvider(FrameCaptureRegistry& registry)
      : registry_{registry} {}

  void Init(const ChangeNotifier& change_notifier);

  std::u16string GetText() const;
  std::optional<scada::aui::Color> GetColor() const;

 private:
  FrameCaptureRegistry& registry_;

  boost::signals2::scoped_connection connection_;
};
