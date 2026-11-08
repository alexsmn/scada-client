#pragma once

#include "aui/color.h"
#include "services/display_selection_registry.h"

#include <boost/signals2/connection.hpp>
#include <functional>
#include <optional>
#include <string>

// The status-strip cell warning that the loaded display runtime is too old to
// mark bad or missing readings on a schematic (backlog 1006).
//
// Empty while there is nothing to say — no display opened yet, or a runtime
// that keeps the quality-marking contract — so the strip is unchanged for
// everyone else. Coloured in the bad-quality token while it shows, because
// what it reports is lost data-quality marking, which is a process signal
// rather than chrome.
class DisplayRuntimeStatusProvider final {
 public:
  using ChangeNotifier = std::function<void()>;

  explicit DisplayRuntimeStatusProvider(DisplaySelectionRegistry& registry)
      : registry_{registry} {}

  void Init(const ChangeNotifier& change_notifier);

  std::u16string GetText() const;
  std::optional<scada::aui::Color> GetColor() const;

 private:
  DisplaySelectionRegistry& registry_;

  boost::signals2::scoped_connection connection_;
};
