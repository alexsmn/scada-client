#pragma once

#include "main_window/pane_modes.h"

#include <QPoint>
#include <QWidget>

#include <functional>
#include <optional>
#include <string>
#include <vector>

class QFrame;
class QToolButton;
class QVBoxLayout;
class QIcon;
class QDragEnterEvent;
class QDragLeaveEvent;
class QDragMoveEvent;
class QDropEvent;
class QEvent;

// Left activity rail — opt-in reshell chrome. A charcoal column in three
// zones, top to bottom (docs/product/ui-mockups/screens/activity-rail.html):
//
//  - the sidebar's pane modes (Objects, Devices, Files, Nodes), which select
//    which panes occupy the left sidebar;
//  - the profile's pages, numbered 1..N, plus a "+" that creates one. Pages
//    stand in for the web client's browser tabs: each replaces the whole
//    workspace. A separator above them is what keeps a page marker from
//    reading as a mode — the web client uses a band there instead;
//  - a stretch, then the pinned utilities (Settings, and Users for an admin
//    session) at the foot.
//
// The rail never opens a workspace tab, and a utility is not a third kind of
// destination: it opens its surface in the current page the way any view does,
// so activating one leaves the page marker lit. Every marker is a projection
// of real state (see MainWindow::RefreshPaneModeMarker), not a record of the
// last click, so none can go stale when a page switch or a manual pane close
// changes what is on screen.
class ActivityBar : public QWidget {
  Q_OBJECT

 public:
  // Dedicated rail glyph drawn for a mode. Kept independent of the view command
  // icons (which are toolbar-shaped) so the rail reads as a coherent,
  // workbench-style icon set.
  enum class Icon {
    kNone,
    kObjects,
    kDevices,
    kFiles,
    kNodes,
    kAdministration,
    kNewPage,
    kSettings,
    kUsers,
  };

  // One rail entry. `icon_kind` selects the dedicated glyph and falls back to
  // the label's first letter when kNone.
  struct Mode {
    PaneModeId id = PaneModeId::kObjects;
    std::u16string label;
    Icon icon_kind = Icon::kNone;
  };

  // One page button. Rendered as the operator's chosen icon, falling back to
  // the 1-based position when no icon is set or the key is unknown. The
  // tooltip carries both — `2 · Alarms` — because titles are arbitrary and
  // will not fit a 48 px rail, and the ordinal still names the shortcut.
  struct PageButton {
    int page_id = 0;
    std::u16string title;
    // A `PageIcon::key` (`main_window/page_icons.h`), or empty for none.
    std::string icon_key;
    // Another main window already has this page open, so activating it would
    // be refused. Shown disabled rather than letting the operator find out
    // through a message box.
    bool opened_elsewhere = false;
  };

  // One pinned utility at the foot of the rail. Unlike a mode or a page, a
  // utility owns no shell state of its own — it opens a view in the current
  // page — so its marker means "that view is what the workspace is showing",
  // and it is set from the active view rather than from the click.
  struct Utility {
    // Identifies the utility to the window, which maps it to a command. Not
    // persisted anywhere, so the values are free to change.
    int utility_id = 0;
    std::u16string label;
    Icon icon_kind = Icon::kNone;
  };

  // Invoked when the user picks a mode.
  using ActivateCallback = std::function<void(PaneModeId)>;
  // Invoked when the user picks a pinned utility.
  using ActivateUtilityCallback = std::function<void(int utility_id)>;
  // Invoked when the user picks a page button.
  using ActivatePageCallback = std::function<void(int page_id)>;
  // Invoked for the "+" button.
  using NewPageCallback = std::function<void()>;
  // Invoked on a right-click over a page button, with a global position for the
  // context menu.
  using PageContextMenuCallback =
      std::function<void(int page_id, const QPoint& global_pos)>;
  // Invoked when a page button is dragged to a new slot. `new_index` is
  // 0-based within the pages group.
  using ReorderPageCallback = std::function<void(int page_id, int new_index)>;

  ActivityBar(QWidget* parent,
              std::vector<Mode> modes,
              ActivateCallback on_activate);
  ~ActivityBar() override;

  // Marks `mode` as active, or clears the marker entirely when nothing is
  // passed — which is what happens when the open panes match no mode.
  void SetActiveMode(std::optional<PaneModeId> mode);

  // Shows or hides a mode's button. Used for the admin-gated Nodes mode: a
  // hidden button says "not yours", where a disabled one would promise a
  // surface that is merely unfinished.
  void SetModeAvailable(PaneModeId mode, bool available);

  // Wires the pages group. Separate from the constructor because pages arrive
  // from the profile, which the window reads after the rail is built.
  void SetPageCallbacks(ActivatePageCallback on_activate_page,
                        NewPageCallback on_new_page,
                        PageContextMenuCallback on_page_context_menu,
                        ReorderPageCallback on_reorder_page);

  // Rebuilds the pages group. Cheap; the window calls it whenever a page is
  // opened, added, renamed or deleted.
  void SetPages(std::vector<PageButton> pages);

  // Marks the open page, or clears the page marker when `page_id` names none.
  void SetActivePage(int page_id);

  // Builds the pinned-utility group at the foot. Called once, after the modes:
  // the utilities are fixed for the session, where the pages are not.
  void SetUtilities(std::vector<Utility> utilities,
                    ActivateUtilityCallback on_activate_utility);

  // Shows or hides a utility. Used for the admin-gated Users button, hidden
  // rather than disabled for the same reason the Nodes mode is.
  void SetUtilityAvailable(int utility_id, bool available);

  // Marks the utility whose view the workspace is showing, or clears the
  // marker when it is showing something else.
  void SetActiveUtility(std::optional<int> utility_id);

  // The drop index a page dragged to `local_y` (in rail coordinates) would land
  // at. Exposed for tests, which cannot synthesize a real drag.
  int PageDropIndexForY(int local_y) const;

  // Positions the drag drop-line as if the cursor were at `local_y`. Exposed
  // for the same reason as PageDropIndexForY: a real drag cannot be
  // synthesized, and where this line lands is the whole point of drawing it.
  void ShowDropIndicatorForTest(int local_y) { ShowDropIndicator(local_y); }

 protected:
  // QWidget — the rail is the drop target for page reordering. Accepting the
  // drop on the container rather than on each button means the gap between
  // buttons, and the area past the last one, are valid drop positions too.
  void dragEnterEvent(QDragEnterEvent* event) override;
  void dragMoveEvent(QDragMoveEvent* event) override;
  void dragLeaveEvent(QDragLeaveEvent* event) override;
  void dropEvent(QDropEvent* event) override;

  // QObject — starts a drag from a page button once the mouse has moved far
  // enough. Filtered rather than subclassed so the page buttons stay ordinary
  // QToolButtons.
  bool eventFilter(QObject* watched, QEvent* event) override;

  // QWidget — recomputes the pages separator when the palette changes, so a
  // live theme switch does not leave the rule coloured for the previous one.
  void changeEvent(QEvent* event) override;

 private:
  // Builds one rail button with the shared sizing and glyph treatment.
  QToolButton* MakeButton(const QIcon& icon, const QString& tooltip);

  // Recomputes the pages separator's colour from the current palette.
  void ApplySeparatorColour();

  // Moves the drag drop-line to the slot `local_y` would drop into, creating
  // it on first use. Without it the drop slot is invisible until the page has
  // already moved.
  void ShowDropIndicator(int local_y);

  // Takes the drop-line back out of the layout, on leave or after a drop.
  void HideDropIndicator();

  struct Item {
    Mode mode;
    QToolButton* button = nullptr;
  };

  struct PageItem {
    PageButton page;
    QToolButton* button = nullptr;
  };

  struct UtilityItem {
    Utility utility;
    QToolButton* button = nullptr;
  };

  std::vector<Item> items_;
  ActivateCallback on_activate_;
  ActivatePageCallback on_activate_page_;
  NewPageCallback on_new_page_;
  PageContextMenuCallback on_page_context_menu_;
  ReorderPageCallback on_reorder_page_;

  std::vector<PageItem> page_items_;
  int active_page_id_ = 0;

  std::vector<UtilityItem> utility_items_;
  ActivateUtilityCallback on_activate_utility_;
  // The rail's own layout, so SetUtilities can append the foot group after the
  // stretch the constructor put in.
  QVBoxLayout* root_layout_ = nullptr;

  // Sizes taken from the platform style at construction (see RailIconSize),
  // so the rail tracks DPI and the OS text-size setting instead of the fixed
  // 52/44/24 it used to carry.
  int icon_size_ = 16;
  int button_size_ = 28;
  int band_inset_ = 2;

  // The pages group's own layout, so SetPages can rebuild just that section.
  QVBoxLayout* pages_layout_ = nullptr;
  // The container the pages group sits in. Purely structural since 2026-08-30
  // — nothing paints it; the separator below is what distinguishes a page
  // marker from a pane-mode marker.
  QWidget* pages_band_ = nullptr;
  // The rule between the pane modes and the pages. The Qt client separates the
  // two groups with this; the web client uses a band instead (shell.md §2.1).
  QFrame* pages_separator_ = nullptr;
  QToolButton* new_page_button_ = nullptr;

  // Where a left-press landed on a page button, so eventFilter can tell a
  // click from the start of a drag.
  QPoint drag_press_pos_;
  int drag_page_id_ = 0;

  // The accent line drawn between page buttons during a reorder drag. Created
  // lazily and kept parented to the band, so it costs nothing until a drag
  // starts.
  QWidget* drop_indicator_ = nullptr;
};
