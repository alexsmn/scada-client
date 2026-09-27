#pragma once

#include "main_window/pane_modes.h"

#include <optional>
#include <string>
#include <string_view>

class Page;

// What PaneModeController needs from the window it drives. Every pane is named
// by its `WindowInfo::name`, so the controller never sees a widget, a view or
// the controller registry, and runs in a test binary without any of them.
class PaneModeHost {
 public:
  virtual ~PaneModeHost() = default;

  // Whether a pane of `pane_type` is open in the current page.
  virtual bool IsPaneOpen(std::string_view pane_type) = 0;
  // Whether the current user may open `pane_type`: false for an unregistered
  // type, or for an admin-gated one the user lacks the right for.
  virtual bool CanOpenPane(std::string_view pane_type) = 0;
  // Closes the pane of `pane_type`. No-op if it is not open.
  virtual void ClosePane(std::string_view pane_type) = 0;
  // Opens `pane_type` without activating it, open by the time this returns.
  virtual void OpenPane(std::string_view pane_type) = 0;
  // Raises the pane of `pane_type` above its tabified siblings, if it is open.
  virtual void FrontPane(std::string_view pane_type) = 0;
  // The `WindowInfo::name` of the active view, or empty when there is none.
  virtual std::string_view ActivePaneType() = 0;
  // The page the window is showing.
  virtual const Page& CurrentPage() = 0;
  // The profile field the chosen mode's key is persisted in.
  virtual std::string& PersistedModeKey() = 0;
  // Rail output: whether `id` is offered, and which mode (if any) is marked.
  virtual void ShowModeAvailable(PaneModeId id, bool available) = 0;
  virtual void ShowActiveMode(std::optional<PaneModeId> id) = 0;
};

// The left sidebar's mode policy: which pane set is on screen, switching
// between sets, persisting the choice, and keeping the activity rail's marker
// true to what is actually open. The mode *table* is `pane_modes.h`, which is
// pure data; this is the part that acts on a window.
//
// Split out of MainWindow on 2026-09-27 (backlog 722). MainWindow is the host
// and forwards the pane close/activate notifications it receives.
class PaneModeController {
 public:
  // `host` must outlive this object.
  explicit PaneModeController(PaneModeHost& host);

  PaneModeController(const PaneModeController&) = delete;
  PaneModeController& operator=(const PaneModeController&) = delete;

  // Switches the sidebar to `id`: closes the panes that do not belong to it,
  // opens the ones that do in declared order, fronts the first, persists the
  // choice and re-derives the marker. Does nothing for a mode the user may
  // not open.
  void SetMode(PaneModeId id);

  // Selects the mode that owns `pane_type`. Returns false when no mode owns it
  // (a workspace view, or the bottom-docked Events pane).
  bool SelectModeForPane(std::string_view pane_type);

  // The mode the window is in: the persisted one if the user may open it,
  // Objects if they may not, and one inferred from the current page if the
  // profile predates the rail and records none.
  PaneModeId ActiveMode();

  // Whether the current user may open `id`. A mode needing admin rights is
  // available only when every one of its panes resolves for this user, which
  // is the same resolution the menus use.
  bool IsModeAvailable(PaneModeId id);

  // Brings the current page's panes into line with the active mode, without
  // touching the persisted choice beyond re-writing it.
  void ApplyToCurrentWindow() { SetMode(ActiveMode()); }

  // Rewrites `page` so its left panes match the active mode, for a page about
  // to be opened. See ApplyPaneModeToPage.
  void ConformPage(Page& page);

  // Raises the active mode's first pane above its tabified siblings.
  void FrontPrimaryPane();

  // Re-derives the rail's per-mode availability and its marker from the panes
  // that are actually open, so the marker cannot go stale.
  void RefreshMarker();

  // The host's notifications. Both ignore what SetMode provokes mid-switch,
  // and panes no mode owns.
  void OnPaneClosed(std::string_view pane_type, bool page_closing);
  void OnPaneActivated(std::string_view pane_type);

 private:
  void FrontPrimaryPane(const PaneMode& mode);

  PaneModeHost& host_;
  // True while SetMode is closing and opening panes, whose notifications would
  // otherwise re-derive the marker from a half-applied set.
  bool applying_ = false;
};
