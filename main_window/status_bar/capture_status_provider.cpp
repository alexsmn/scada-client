#include "main_window/status_bar/capture_status_provider.h"

#include "aui/severity_colors.h"
#include "aui/translation.h"
#include "base/utf_convert.h"

#include <string>

void CaptureStatusProvider::Init(const ChangeNotifier& change_notifier) {
  connection_ = registry_.SubscribeChanged(change_notifier);
}

std::u16string CaptureStatusProvider::GetText() const {
  const std::span<const FrameCaptureRegistry::ArmedDevice> armed =
      registry_.armed();
  if (armed.empty())
    return {};

  // One device is named; several are counted. Naming only the first would read
  // as "that one device", which is exactly the capture the operator would then
  // fail to stop.
  if (armed.size() == 1) {
    return Translate("Capturing") + u" · " + armed.front().display_name;
  }
  return Translate("Capturing") + u" · " +
         UtfConvert<char16_t>(std::to_string(armed.size()));
}

std::optional<scada::aui::Color> CaptureStatusProvider::GetColor() const {
  if (registry_.armed().empty())
    return std::nullopt;
  // Reuses the critical severity token: a running capture is not an alarm, but
  // it is the one thing in the strip the operator must act on, and this is the
  // colour the strip already uses to mean that.
  return scada::aui::SeverityColor(scada::aui::SeverityLevel::kCritical);
}
