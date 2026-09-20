#pragma once

#include "services/display_selection_registry.h"

#include <boost/signals2/connection.hpp>
#include <functional>
#include <string>

// The status-strip cell naming what the operator has selected on a schematic
// display ("Selected · Q1").
//
// Empty while nothing is selected, so the strip is unchanged until the
// operator clicks something — and uncoloured, because a selection is the
// operator's own doing rather than a state they have to act on. That is the
// distinction from `CaptureStatusProvider`, which colours its cell precisely
// because an armed capture is something left running by mistake.
//
// Matches `docs/product/ui-mockups/screens/substation-display.html`, whose
// status strip carries `Selected Q1` between the server cell and the bay.
class SelectionStatusProvider final {
 public:
  using ChangeNotifier = std::function<void()>;

  explicit SelectionStatusProvider(DisplaySelectionRegistry& registry)
      : registry_{registry} {}

  void Init(const ChangeNotifier& change_notifier);

  std::u16string GetText() const;

 private:
  DisplaySelectionRegistry& registry_;

  boost::signals2::scoped_connection connection_;
};
