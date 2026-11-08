#include "main_window/status_bar/display_runtime_status_provider.h"

#include "aui/severity_colors.h"

void DisplayRuntimeStatusProvider::Init(const ChangeNotifier& change_notifier) {
  connection_ = registry_.SubscribeChanged(change_notifier);
}

std::u16string DisplayRuntimeStatusProvider::GetText() const {
  // Already translated by whoever published it: the display module owns the
  // wording, since it is the one that knows what the runtime lacks.
  return registry_.runtime_notice();
}

std::optional<scada::aui::Color> DisplayRuntimeStatusProvider::GetColor()
    const {
  if (registry_.runtime_notice().empty())
    return std::nullopt;
  // The bad-quality token: the cell reports that bad quality may go unmarked,
  // and quality colours are fixed process semantics, not chrome.
  return scada::aui::QualityColor(scada::aui::Quality::kBad);
}
