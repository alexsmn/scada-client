#include "main_window/pane_modes.h"

#include "base/check.h"
#include "base/no_destructor.h"
#include "profile/page.h"

#include <algorithm>
#include <limits>

namespace {

// std::ranges::contains is not available in every toolchain the client builds
// with, so spell the membership test once here.
bool Contains(std::span<const std::string_view> haystack,
              std::string_view needle) {
  return std::ranges::find(haystack, needle) != haystack.end();
}

// The rail's mode table. `Portfolio` follows `Struct` and `Favorites` follows
// `FileSystemView` so the mode's own subject is the pane that ends up fronted.
const std::vector<PaneMode>& PaneModeTable() {
  static const scada::base::NoDestructor<std::vector<PaneMode>> modes{
      std::vector<PaneMode>{
          {.id = PaneModeId::kObjects,
           .key = "objects",
           .label = "Objects",
           .pane_types = {"Struct", "Portfolio"}},
          {.id = PaneModeId::kDevices,
           .key = "devices",
           .label = "Devices",
           .pane_types = {"Subsystems"}},
          {.id = PaneModeId::kFiles,
           .key = "files",
           .label = "Files",
           .pane_types = {"FileSystemView", "Favorites"}},
          {.id = PaneModeId::kNodes,
           .key = "nodes",
           .label = "Nodes",
           .pane_types = {"Nodes"},
           .requires_admin = true},
          // The admin surfaces the "More" menu used to be the only way into:
          // users, access rights and the configuration tables that carry
          // WIN_REQUIRES_ADMIN. Admin-gated the same way Nodes is — hidden
          // rather than disabled, so an operator is never shown a door they
          // cannot open.
          {.id = PaneModeId::kAdministration,
           .key = "administration",
           .label = "Administration",
           .pane_types = {"Administration"},
           .requires_admin = true},
      }};
  return *modes;
}

}  // namespace

std::span<const PaneMode> GetPaneModes() {
  return PaneModeTable();
}

const PaneMode& GetPaneMode(PaneModeId id) {
  for (const PaneMode& mode : PaneModeTable()) {
    if (mode.id == id)
      return mode;
  }
  scada::base::NotReached("Unknown pane mode id");
}

const PaneMode* FindPaneModeByKey(std::string_view key) {
  for (const PaneMode& mode : PaneModeTable()) {
    if (mode.key == key)
      return &mode;
  }
  return nullptr;
}

const PaneMode* FindPaneModeOwningPaneType(std::string_view window_type) {
  for (const PaneMode& mode : PaneModeTable()) {
    if (Contains(mode.pane_types, window_type))
      return &mode;
  }
  return nullptr;
}

std::span<const std::string_view> GetModeOwnedPaneTypes() {
  static const scada::base::NoDestructor<std::vector<std::string_view>> types{
      [] {
        std::vector<std::string_view> result;
        for (const PaneMode& mode : PaneModeTable()) {
          result.insert(result.end(), mode.pane_types.begin(),
                        mode.pane_types.end());
        }
        return result;
      }()};
  return *types;
}

PaneModeDelta ComputePaneModeDelta(
    const PaneMode& target,
    std::span<const std::string_view> currently_open) {
  PaneModeDelta delta;

  for (std::string_view open_type : currently_open) {
    if (Contains(target.pane_types, open_type))
      continue;
    // Only panes some mode claims are the rail's business; anything else the
    // operator opened stays untouched.
    if (!Contains(GetModeOwnedPaneTypes(), open_type))
      continue;
    delta.to_close.push_back(open_type);
  }

  for (std::string_view wanted : target.pane_types) {
    if (Contains(currently_open, wanted))
      continue;
    delta.to_open.push_back(wanted);
  }

  return delta;
}

void ApplyPaneModeToPage(Page& page, const PaneMode& mode) {
  for (std::string_view pane_type : GetModeOwnedPaneTypes()) {
    const bool wanted = Contains(mode.pane_types, pane_type);

    WindowDefinition* existing = nullptr;
    for (int i = 0; i < page.GetWindowCount(); ++i) {
      WindowDefinition& window = page.GetWindow(i);
      if (window.type == pane_type) {
        existing = &window;
        break;
      }
    }

    if (existing) {
      // Never delete: the definition carries the window id the dock-state blob
      // keys on, plus the pane's stored parameters.
      existing->visible = wanted;
      continue;
    }

    if (!wanted)
      continue;

    // The page predates this pane (or never used it). A bare type is enough —
    // the view manager resolves the WindowInfo (and its default size) when it
    // creates the view, the same way MakeOverviewPage seeds its panes.
    page.AddWindow(WindowDefinition{pane_type});
  }
}

PaneModeId InferPaneModeFromPage(const Page& page) {
  std::vector<std::string_view> visible;
  for (int i = 0; i < page.GetWindowCount(); ++i) {
    const WindowDefinition& window = page.GetWindow(i);
    if (window.visible && Contains(GetModeOwnedPaneTypes(), window.type)) {
      visible.push_back(window.type);
    }
  }

  // Score each mode by the panes it shares with the page minus the ones it
  // would have to open, and require at least one shared pane. A page that
  // matches nothing (it predates the rail, or holds only workspace windows)
  // falls through to Objects rather than to whichever mode happens to open the
  // fewest panes. Ties resolve to the first mode in rail order.
  PaneModeId best = PaneModeId::kObjects;
  int best_score = std::numeric_limits<int>::min();
  for (const PaneMode& mode : PaneModeTable()) {
    int shared = 0;
    int missing = 0;
    for (std::string_view pane_type : mode.pane_types) {
      if (Contains(visible, pane_type))
        ++shared;
      else
        ++missing;
    }
    const int score = shared - missing;
    if (shared > 0 && score > best_score) {
      best_score = score;
      best = mode.id;
    }
  }
  return best;
}
