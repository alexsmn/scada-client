#include "services/display_selection_registry.h"

#include <utility>

void DisplaySelectionRegistry::SetSelection(Owner owner, std::u16string label) {
  if (label.empty()) {
    ClearSelection(owner);
    return;
  }

  if (owner_ == owner && label_ == label)
    return;

  owner_ = owner;
  label_ = std::move(label);
  changed_signal_();
}

void DisplaySelectionRegistry::ClearSelection(Owner owner) {
  // Not ours to clear: another display published after us, and blanking the
  // cell here would take away the readout for the display the operator is
  // looking at.
  if (owner_ != owner)
    return;

  if (label_.empty()) {
    owner_ = nullptr;
    return;
  }

  owner_ = nullptr;
  label_.clear();
  changed_signal_();
}
