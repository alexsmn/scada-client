#pragma once

#include <span>
#include <string_view>
#include <vector>

class Page;

// The left sidebar's mode vocabulary.
//
// The activity rail selects one of these; a mode names the set of `WIN_SING`
// panes that occupy the left dock while it is active. A mode never opens a
// workspace tab and never replaces the page — that separation is the whole
// point of the vocabulary, and it mirrors the web client's sidebar modes so the
// two shells stay describable in the same words.
enum class PaneModeId {
  kObjects,
  kDevices,
  kFiles,
  kNodes,
};

// One rail mode. `pane_types` holds `WindowInfo::name` values in the order they
// should be docked — the order is load-bearing, because the view manager
// tabifies each new dock onto the first one already in the area, so the first
// entry becomes the fronted pane.
struct PaneMode {
  PaneModeId id;
  // Stable key written to the profile. Decoupled from the enumerator so
  // reordering `PaneModeId` cannot silently repoint saved profiles.
  std::string_view key;
  // English source string; call sites pass it through Translate().
  const char* label;
  std::vector<std::string_view> pane_types;
  bool requires_admin = false;
};

// The mode table, in rail order.
std::span<const PaneMode> GetPaneModes();

const PaneMode& GetPaneMode(PaneModeId id);

// Resolves a persisted key. Returns nullptr for an unknown value, so a profile
// written by another build degrades to the caller's default.
const PaneMode* FindPaneModeByKey(std::string_view key);

// The mode that owns `window_type`, or nullptr if no mode does. Used to map an
// active pane back to a rail mode.
const PaneMode* FindPaneModeOwningPaneType(std::string_view window_type);

// Every pane type claimed by some mode — i.e. the left-dock panes the rail is
// authoritative over. Deliberately excludes the `Event` pane, which is
// `WIN_DOCKB`, lives in the bottom dock, and is driven by the event
// auto-show/hide policy in MainWindowModule.
std::span<const std::string_view> GetModeOwnedPaneTypes();

// The pane changes needed to reach `target` from the currently open set, as
// `WindowInfo::name` values. Names rather than `WindowInfo` pointers, so the
// whole mode model stays independent of the controller registry — which is
// populated by the running app, not by test binaries.
struct PaneModeDelta {
  std::vector<std::string_view> to_close;
  std::vector<std::string_view> to_open;
};

// Computes the delta. Callers must apply every `to_close` before any
// `to_open`, and apply `to_open` in order: the view manager tabifies onto the
// first dock present in the area, so opening before closing would tab the new
// panes onto ones that are about to disappear.
PaneModeDelta ComputePaneModeDelta(
    const PaneMode& target,
    std::span<const std::string_view> currently_open);

// Rewrites `page` so its left panes match `mode`: in-mode panes become visible
// (a definition is created only if the page has none), out-of-mode panes become
// hidden. Definitions are never deleted — that preserves each pane's window id,
// which the dock-state blob keys on, and its stored parameters.
void ApplyPaneModeToPage(Page& page, const PaneMode& mode);

// Guesses the mode that best describes a page's currently visible panes. Used
// once per profile, when a page predates the rail and no mode was persisted, so
// the operator lands on the panes they had rather than on an arbitrary default.
PaneModeId InferPaneModeFromPage(const Page& page);
