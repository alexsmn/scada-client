#pragma once

#include "base/lifetime.h"

#include <boost/signals2/connection.hpp>
#include <boost/signals2/signal.hpp>
#include <functional>
#include <string>

// What the operator has selected on a schematic display, for the chrome that
// reports it — today the status strip's `Selected Q1` cell.
//
// The selection itself is drawn by the display widget, which owns the hit test
// and the page geometry. This registry exists because the *readout* is not the
// view's to draw: `docs/product/ui-mockups/screens/substation-display.html`
// puts it in the application status strip, which is built once for the window
// and cannot reach into whichever display happens to be open.
//
// **Last selection wins, and a view clears only its own.** Several display
// views can be open at once and nothing here tracks which is focused, so the
// rule is the simplest one that cannot lie: selecting publishes, and a view
// that is closing (or whose operator clicked bare page) clears the cell only
// if the cell is still showing *its* selection. Without the owner check a
// background view's teardown would blank a readout belonging to the display
// the operator is actually looking at.
class DisplaySelectionRegistry {
 public:
  // Identifies the publishing view. Any stable per-view address will do; the
  // registry compares it and never dereferences it.
  using Owner = const void*;

  using ChangeCallback = std::function<void()>;

  // Publishes `label` as the current selection. An empty label clears, which
  // is what a click on bare page produces.
  void SetSelection(Owner owner, std::u16string label);

  // Clears the selection if `owner` is the one that published it, and does
  // nothing otherwise. Idempotent, so a view may call it on both a cleared
  // selection and its own teardown.
  void ClearSelection(Owner owner);

  // Empty when nothing is selected.
  const std::u16string& label() const SCADA_LIFETIME_BOUND { return label_; }

  [[nodiscard]] boost::signals2::scoped_connection SubscribeChanged(
      const ChangeCallback& callback) {
    return changed_signal_.connect(callback);
  }

 private:
  Owner owner_ = nullptr;
  std::u16string label_;

  boost::signals2::signal<void()> changed_signal_;
};
