#pragma once

#include "main_window/pane_modes.h"

#include <QPoint>
#include <QToolBar>

#include <functional>
#include <optional>
#include <string>
#include <vector>

class QAction;
class QToolButton;
class QIcon;
class QDragEnterEvent;
class QDragLeaveEvent;
class QDragMoveEvent;
class QDropEvent;
class QEvent;

// Left activity rail. A real `QToolBar` of `QAction`s
// in `Qt::LeftToolBarArea`, in three zones, top to bottom
// (docs/product/ui-mockups/screens/activity-rail.html):
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
//
// **A QToolBar of QActions, but still stylesheet-painted** — the partial of
// shell.md §9, and the split is deliberate. Being a real toolbar of real
// actions is what makes the rail hideable through the standard toolbar context
// menu and its modes reachable as commands. Handing the *painting* to the
// platform style as well, which §9 also asked for, was measured and rejected:
// the style draws a checked action as a neutral rounded highlight with no
// accent, and a separator as a faint dotted line — losing both the VS Code
// active marker and the separator contrast this rail is required to have. So
// the structure is native and the marker and separator keep their sheet.
class ActivityBar : public QToolBar {
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
  // Builds one rail action with the shared glyph treatment.
  QAction* MakeAction(const QIcon& icon, const QString& tooltip);

  // The tool button the toolbar made for `action`. Null before the toolbar has
  // laid the action out; every caller here runs after that.
  QToolButton* ButtonFor(const QAction* action) const;

  // Rebuilds the rail's stylesheet from the current palette and tokens. Holds
  // the marker and the separator, so a live theme switch has to re-run it.
  void ApplyStyleSheet();

  // Moves the drag drop-line to the slot `local_y` would drop into, creating
  // it on first use. Without it the drop slot is invisible until the page has
  // already moved.
  void ShowDropIndicator(int local_y);

  // Takes the drop-line back out of the layout, on leave or after a drop.
  void HideDropIndicator();

  struct Item {
    Mode mode;
    QAction* action = nullptr;
  };

  struct PageItem {
    PageButton page;
    QAction* action = nullptr;
  };

  struct UtilityItem {
    Utility utility;
    QAction* action = nullptr;
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

  // Re-entry guard for ApplyStyleSheet: setting the sheet on this widget
  // delivers a PaletteChange back to it, which would otherwise recompute the
  // sheet again, without end.
  bool applying_style_sheet_ = false;

  // The rule between the pane modes and the pages, as a toolbar separator
  // painted by the rail's own sheet. The Qt client separates the two groups
  // with this; the web client uses a band instead (shell.md §2.1).
  QAction* separator_action_ = nullptr;
  // The "+", and the expanding spacer that pins the utilities to the foot.
  // New pages are inserted before the "+", so both are also the anchors
  // SetPages rebuilds against.
  QAction* new_page_action_ = nullptr;
  QAction* spacer_action_ = nullptr;

  // Where a left-press landed on a page button, so eventFilter can tell a
  // click from the start of a drag.
  QPoint drag_press_pos_;
  int drag_page_id_ = 0;

  // The accent line drawn between page buttons during a reorder drag. Created
  // lazily and kept parented to the band, so it costs nothing until a drag
  // starts.
  QWidget* drop_indicator_ = nullptr;
};
