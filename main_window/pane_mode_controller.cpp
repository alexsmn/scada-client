#include "main_window/pane_mode_controller.h"

#include "base/auto_reset.h"

#include <algorithm>
#include <vector>

namespace {

// What the sidebar shows right now, in the rail's vocabulary and table order.
std::vector<std::string_view> OpenModePanes(PaneModeHost& host) {
  std::vector<std::string_view> open_panes;
  for (std::string_view pane_type : GetModeOwnedPaneTypes()) {
    if (host.IsPaneOpen(pane_type))
      open_panes.push_back(pane_type);
  }
  return open_panes;
}

}  // namespace

PaneModeController::PaneModeController(PaneModeHost& host) : host_{host} {}

void PaneModeController::SetMode(PaneModeId id) {
  if (!IsModeAvailable(id))
    return;

  const PaneMode& mode = GetPaneMode(id);
  const PaneModeDelta delta = ComputePaneModeDelta(mode, OpenModePanes(host_));

  {
    // Closing and opening panes fires the host's close/activate
    // notifications; let the switch finish before re-deriving the marker from
    // a half-applied set.
    scada::base::AutoReset<bool> applying{&applying_, true};

    // Close first, then open in declared order — the view manager tabifies
    // onto the first dock already in the area, so opening early would tab the
    // new panes onto ones that are about to disappear.
    for (std::string_view pane_type : delta.to_close)
      host_.ClosePane(pane_type);
    for (std::string_view pane_type : delta.to_open)
      host_.OpenPane(pane_type);

    FrontPrimaryPane(mode);
  }

  host_.PersistedModeKey() = std::string{mode.key};
  RefreshMarker();
}

bool PaneModeController::SelectModeForPane(std::string_view pane_type) {
  const PaneMode* mode = FindPaneModeOwningPaneType(pane_type);
  if (!mode)
    return false;
  SetMode(mode->id);
  return true;
}

PaneModeId PaneModeController::ActiveMode() {
  if (const PaneMode* mode = FindPaneModeByKey(host_.PersistedModeKey())) {
    // A profile can carry a mode the current user may not open. Fall back
    // rather than presenting an empty sidebar with no way out.
    if (!mode->requires_admin || IsModeAvailable(mode->id))
      return mode->id;
    return PaneModeId::kObjects;
  }
  // No mode recorded — this profile predates the rail. Infer one from the page
  // so the operator keeps the panes they had.
  return InferPaneModeFromPage(host_.CurrentPage());
}

bool PaneModeController::IsModeAvailable(PaneModeId id) {
  const PaneMode& mode = GetPaneMode(id);
  if (!mode.requires_admin)
    return true;
  // The host asks the same resolution the menus use, so the rail and the More
  // menu agree by construction: a WIN_REQUIRES_ADMIN view resolves to no
  // handler without the Configure right.
  return std::ranges::all_of(mode.pane_types, [this](std::string_view type) {
    return host_.CanOpenPane(type);
  });
}

void PaneModeController::ConformPage(Page& page) {
  ApplyPaneModeToPage(page, GetPaneMode(ActiveMode()));
}

void PaneModeController::FrontPrimaryPane() {
  FrontPrimaryPane(GetPaneMode(ActiveMode()));
}

void PaneModeController::FrontPrimaryPane(const PaneMode& mode) {
  if (!mode.pane_types.empty())
    host_.FrontPane(mode.pane_types.front());
}

void PaneModeController::RefreshMarker() {
  for (const PaneMode& mode : GetPaneModes())
    host_.ShowModeAvailable(mode.id, IsModeAvailable(mode.id));

  const std::vector<std::string_view> open_panes = OpenModePanes(host_);

  // An exact match is the honest answer; anything else and the sidebar is not
  // showing a mode, so the rail must not claim one.
  for (const PaneMode& mode : GetPaneModes()) {
    if (std::ranges::equal(mode.pane_types, open_panes)) {
      host_.ShowActiveMode(mode.id);
      return;
    }
  }

  // Partial match: fall back to the mode owning whatever pane is active, so a
  // manually closed sibling still leaves the rail pointing somewhere true.
  if (const PaneMode* mode =
          FindPaneModeOwningPaneType(host_.ActivePaneType())) {
    host_.ShowActiveMode(mode->id);
    return;
  }

  host_.ShowActiveMode(std::nullopt);
}

void PaneModeController::OnPaneClosed(std::string_view pane_type,
                                      bool page_closing) {
  if (applying_ || page_closing || !FindPaneModeOwningPaneType(pane_type))
    return;
  RefreshMarker();
}

void PaneModeController::OnPaneActivated(std::string_view pane_type) {
  if (applying_ || !FindPaneModeOwningPaneType(pane_type))
    return;
  RefreshMarker();
}
