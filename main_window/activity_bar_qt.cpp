#include "main_window/activity_bar_qt.h"

#include "aui/qt/image_util.h"

#include "aui/qt/theme_qt.h"
#include "aui/severity_colors.h"
#include "aui/translation.h"

#include <QApplication>
#include <QDrag>
#include <QDragEnterEvent>
#include <QDragLeaveEvent>
#include <QDropEvent>
#include <QFont>
#include <QFrame>
#include <QIcon>
#include <QMimeData>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPen>

#include <QPixmap>
#include <QStyle>
#include <QToolButton>
#include <QVBoxLayout>
#include <string_view>

#include <algorithm>
#include <ranges>

namespace {

// The drag payload for page reordering. A private MIME type, so a drag from
// somewhere else in the app can never be mistaken for a page.
constexpr char kPageDragMimeType[] = "application/x-scada-rail-page";

// The rail's geometry matches VS Code's activity bar on macOS, which is what
// the user asked for. Read out of the shipped app rather than from memory —
// `/Applications/Visual Studio
// Code.app/Contents/Resources/app/out/vs/workbench/
// workbench.desktop.main.css`, verified 2026-08-30:
//
//   --activity-bar-width:        48px   (rail, and each action's width)
//   --activity-bar-action-height:48px   (so an item is a 48 square)
//   --activity-bar-icon-size:    24px   (codicon font-size)
//   active item indicator:       border-left: 2px solid, top 0, height 100%
//
// **The icon is 20 here, not 24, and that is deliberate.** VS Code's 24 is a
// *font-size* for a codicon, and an icon font leaves bearing inside the em box
// — the drawn ink is nearer 18-19px. Our Lucide glyphs are SVGs that use the
// full 24px viewBox edge to edge, so rendering them at 24 puts noticeably more
// ink on screen than VS Code shows at the same nominal number. 20 matches the
// *ink*, which is what "the same as VS Code" means to look at, and it is also
// the smaller icon that was asked for first.
constexpr int kRailWidth = 48;
constexpr int kButtonWidth = 48;
constexpr int kIconSize = 20;
// Shorter than it is wide, and that is the point. VS Code's item is a 48
// square around a 24 glyph — half the box is the glyph. Ours draws a 20 glyph
// (see above: the same nominal size in an icon font puts less ink on screen
// than an edge-to-edge SVG), so a 48-tall item left proportionally far more
// air above and below than VS Code shows, and the column read as stacked
// slabs rather than a list of marks. Keeping VS Code's glyph-to-item ratio
// vertically is what fixes it; the width stays 48 because that is the rail.
constexpr int kButtonHeight = kIconSize * 2;
// The rail's own right edge. VS Code draws one — `.activitybar.bordered:before`
// is `border-right-width: 1px` — and without it the rail and the Explorer
// beside it are the *same* colour: both measured #1e1e1e on the dark theme
// 2026-08-30, with nothing between them, so the two surfaces ran together.
constexpr int kRailEdgeWidth = 1;
// VS Code's active-item indicator. Ours was 3px.
constexpr int kActiveMarkerWidth = 2;
// The pages band spans the rail's full width, because a 48px button leaves no
// room to inset it. It separates by rule and fill rather than by being a box,
// so the rules do the work a box outline would have done.
constexpr int kBandPadding = 2;
// The rule between the mode group and the pages group. 1px is what a
// native separator is; the contrast, not the thickness, is what makes it
// read (see ApplySeparatorColour).
constexpr int kSeparatorThickness = 1;
// Air above and below the band. Contrast alone left the two groups touching;
// the gap is what makes them read as two groups rather than one banded list.
constexpr int kBandGap = 8;

// The drag drop-line. Thin enough to read as a boundary between two buttons
// rather than as a slot of its own.
constexpr int kDropIndicatorHeight = 2;

// The design-token set for the active theme. The rail is only built when a
// token theme is active (see MainWindow), so mapping the legacy case to dark is
// a harmless fallback.
const scada::aui::ThemeTokens& RailTokens() {
  scada::aui::Theme theme = scada::aui::Theme::kDark;
  switch (scada::aui::GetSeverityTheme()) {
    case scada::aui::SeverityTheme::kLight:
      theme = scada::aui::Theme::kLight;
      break;
    case scada::aui::SeverityTheme::kHighContrast:
      theme = scada::aui::Theme::kHighContrast;
      break;
    default:
      break;
  }
  return scada::aui::GetThemeTokens(theme);
}

// The Lucide glyph for a rail section (docs/client/ux/iconography.md §5.3).
//
// These were hand-drawn QPainterPath marks at a 1.8 px pen — someone
// reimplementing Feather/Lucide geometry in C++ because there was no set to
// draw from. There is one now, so the rail reads from the same assets as
// everything else: one stroke weight, one grid, and glyphs that stay crisp at
// fractional scaling instead of a pen width chosen for 24 px.
std::string_view ModeGlyph(ActivityBar::Icon kind) {
  switch (kind) {
    // A tree of objects, which is what the pane shows.
    case ActivityBar::Icon::kObjects:
      return ":/icons/folder-tree.svg";
    // The same noun the hardware tree uses for a device (§5.2), so the rail
    // and the pane it opens agree.
    case ActivityBar::Icon::kDevices:
      return ":/icons/router.svg";
    // `files`, not `folder`: a folder is the container glyph inside the trees,
    // and this is a section of files rather than one folder.
    case ActivityBar::Icon::kFiles:
      return ":/icons/files.svg";
    // Vertices joined by edges — the address space, and deliberately distinct
    // from the single device mark above.
    case ActivityBar::Icon::kNodes:
      return ":/icons/waypoints.svg";
    // A shield carrying a person: the security surface, and the same mark the
    // admin screen uses for the pane mode (users-admin.html).
    case ActivityBar::Icon::kAdministration:
      return ":/icons/shield-user.svg";
    case ActivityBar::Icon::kNewPage:
      return ":/icons/plus.svg";
    // The pinned utilities. `settings` is the same cog the page-icon picker
    // offers, and `user` the same person mark the admin surfaces use, so the
    // foot reads as part of the same set rather than as borrowed chrome.
    case ActivityBar::Icon::kSettings:
      return ":/icons/settings.svg";
    case ActivityBar::Icon::kUsers:
      return ":/icons/user.svg";
    case ActivityBar::Icon::kNone:
      return {};
  }
  return {};
}

// The Lucide glyph for a page icon key (`main_window/page_icons.h`). Every key
// in `GetPageIcons()` must have an arm here; `activity_bar_unittest.cpp` holds
// the two in step. Returns empty for an unknown key, which is what makes a
// profile written by a newer build fall back to the ordinal instead of drawing
// a blank button.
std::string_view PageGlyph(std::string_view key) {
  if (key == "overview")
    return ":/icons/workflow.svg";
  // The alarm mark is the ISA-18.2 triangle, the same shape the severity chips
  // use — a page of alarms should be recognisable as such at a glance.
  if (key == "alarms")
    return ":/icons/triangle-alert.svg";
  if (key == "trend")
    return ":/icons/chart-spline.svg";
  // A substation is a node in the network, not a single device: `network`
  // rather than the `router` mark the Devices mode uses.
  if (key == "substation")
    return ":/icons/network.svg";
  if (key == "table")
    return ":/icons/table.svg";
  if (key == "objects")
    return ":/icons/folder-tree.svg";
  if (key == "devices")
    return ":/icons/router.svg";
  // A log of frames over time, distinct from the device mark above.
  if (key == "devicelog")
    return ":/icons/logs.svg";
  // Re-transmission forwards a value on to somewhere else.
  if (key == "transmission")
    return ":/icons/radio-tower.svg";
  if (key == "files")
    return ":/icons/files.svg";
  if (key == "report")
    return ":/icons/clipboard-list.svg";
  if (key == "settings")
    return ":/icons/settings.svg";
  return {};
}

// A rail button icon that always shows something: the dedicated section glyph,
// or a charcoal-friendly glyph of the label's first letter when the section has
// no dedicated icon.
QIcon ModeIcon(const ActivityBar::Mode& mode, const QColor& fg, int icon_size) {
  QPixmap pixmap{icon_size, icon_size};
  pixmap.fill(Qt::transparent);
  QPainter painter{&pixmap};
  painter.setRenderHint(QPainter::Antialiasing);

  if (const std::string_view glyph = ModeGlyph(mode.icon_kind);
      !glyph.empty()) {
    painter.end();
    return LoadTintedGlyph(glyph, icon_size, fg,
                           qApp ? qApp->devicePixelRatio() : 1.0);
  }

  painter.setPen(fg);
  QFont font = painter.font();
  font.setPixelSize(icon_size * 3 / 4);
  font.setBold(true);
  painter.setFont(font);
  const QString glyph =
      mode.label.empty()
          ? QStringLiteral("?")
          : QString::fromStdU16String(mode.label.substr(0, 1)).toUpper();
  painter.drawText(pixmap.rect(), Qt::AlignCenter, glyph);
  return QIcon{pixmap};
}

// A rail button icon showing `text` — used for the numbered page buttons,
// whose titles are arbitrary and cannot fit a 48 px rail.
QIcon TextIcon(const QString& text, const QColor& fg, int icon_size) {
  QPixmap pixmap{icon_size, icon_size};
  pixmap.fill(Qt::transparent);
  QPainter painter{&pixmap};
  painter.setRenderHint(QPainter::Antialiasing);
  painter.setPen(fg);
  QFont font = painter.font();
  font.setPixelSize(icon_size * 2 / 3);
  font.setBold(true);
  painter.setFont(font);
  painter.drawText(pixmap.rect(), Qt::AlignCenter, text);
  return QIcon{pixmap};
}

}  // namespace

ActivityBar::ActivityBar(QWidget* parent,
                         std::vector<Mode> modes,
                         ActivateCallback on_activate)
    : QToolBar{parent}, on_activate_{std::move(on_activate)} {
  setObjectName(QStringLiteral("activityBar"));
  setOrientation(Qt::Vertical);
  setIconSize({kIconSize, kIconSize});
  // Fixed rather than draggable: the rail is a fixed edge of the workbench,
  // and a movable toolbar would also grow a drag handle the design has no room
  // for. Hiding it stays available — QMainWindow's toolbar context menu lists
  // a toolbar whether or not it is movable, which is the whole point of being
  // one (shell.md §9).
  setMovable(false);
  setFloatable(false);
  // The edge is drawn outside the 48px of button, so the buttons keep VS
  // Code's width and the rule sits beside them.
  setFixedWidth(kRailWidth + kRailEdgeWidth);
  setAcceptDrops(true);

  ApplyStyleSheet();

  const scada::aui::ThemeTokens& tokens = RailTokens();

  for (const Mode& mode : modes) {
    QAction* action = MakeAction(ModeIcon(mode, tokens.fg_on_dark, kIconSize),
                                 QString::fromStdU16String(mode.label));
    const PaneModeId id = mode.id;
    items_.emplace_back(Item{mode, action});
    connect(action, &QAction::triggered, this, [this, id] {
      if (on_activate_)
        on_activate_(id);
    });
  }

  // A pane mode and a page are active at the same time and the two markers are
  // drawn identically, so something has to say which is which. **In the Qt
  // client that is a separator; the web client keeps the band** (see
  // docs/client/ux/shell.md §2.1).
  //
  // The band was tried here first, twice, and it does not carry at this rail's
  // density. Measured on the dark theme 2026-08-30: as a fill alone it stood
  // 8/255 off the rail, and strengthened to a fill plus 1px rules it was still
  // only 15 off with the rules 34 above the fill — legible in a magnified crop
  // and not legible in the running client, which is the only test that counts.
  // A 48px-wide tint has too little area to register as a container. A rule
  // spends all its contrast on one line instead of spreading it over a region,
  // which is why it wins here and why the mockup's band, drawn at a larger
  // scale, does not transfer.
  addSeparator();

  new_page_action_ =
      MakeAction(ModeIcon(Mode{.label = u"+", .icon_kind = Icon::kNewPage},
                          tokens.fg_on_dark, kIconSize),
                 QString::fromStdU16String(Translate("New page")));
  // The "+" is an action, not a destination — it must never carry a marker.
  new_page_action_->setCheckable(false);
  connect(new_page_action_, &QAction::triggered, this, [this] {
    if (on_new_page_)
      on_new_page_();
  });

  // The stretch that pins the utilities to the foot however tall the pages
  // group grows. A toolbar has no addStretch(), so it is an expanding widget.
  auto* spacer = new QWidget{this};
  spacer->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
  addWidget(spacer);

  SetPages({});
}

ActivityBar::~ActivityBar() = default;

void ActivityBar::ApplyStyleSheet() {
  applying_style_sheet_ = true;
  const scada::aui::ThemeTokens& tokens = RailTokens();

  // Derived from the rail's own window colour so it follows the platform
  // palette, the OS light/dark preference and the high-contrast palette, where
  // a baked value would either vanish or shout.
  //
  // Pitched hard. Two earlier attempts put this line 18 and then 49 levels
  // above the rail and both were reported invisible in the running client, so
  // the factor here is chosen to clear that by a wide margin rather than to be
  // tasteful: on the dark theme's #1e1e1e rail it lands near 130. The platform
  // style's own separator — a faint dotted line — is fainter still, which is
  // why this stays in the sheet rather than being handed to the style with the
  // rest of §9.
  const QColor base = palette().color(QPalette::Window);
  const QColor rule =
      base.lightness() < 128 ? base.lighter(430) : base.darker(190);

  // Marker and separator only. Everything structural — the toolbar itself, its
  // hiding, its actions — is now the platform's (shell.md §9, partial).
  setStyleSheet(
      QStringLiteral(
          "#activityBar { background: %1; border: none; "
          "border-right: %9px solid %5; spacing: 2px; }"
          // Both side borders are reserved, and only the left one is ever
          // coloured. A marker drawn as a left border alone steals its width
          // from the content box on one side only, which pushed every glyph
          // 1px right of the rail's centre (measured 24.5 against 23.5,
          // 2026-08-30) — visible as a rail whose icons sit off-axis.
          "#activityBar QToolButton { border: none; "
          "border-left: %6px solid transparent; "
          "border-right: %6px solid transparent; background: transparent; "
          "min-width: %7px; min-height: %8px; }"
          "#activityBar QToolButton:hover { background: %2; }"
          "#activityBar QToolButton:checked { border-left: %6px solid %3; "
          "background: %4; }"
          "#activityBar::separator { background: %5; height: 1px; "
          "margin: %10px 0; }")
          .arg(tokens.rail_bg.name(), tokens.surface_muted.name(),
               tokens.accent.name(),
               // HexArgb, because `accent_soft` carries an alpha (.15 dark,
               // .10 light, and a derived one under the system palette) that
               // the default #RRGGBB name() drops. Dropped, the active marker
               // renders as a solid accent block at ~6.7x its intended
               // strength — and byte-identical to the accent edge drawn over
               // it, so the edge vanishes. Every other consumer of this token
               // already names it this way (`aui/qt/grid.cpp`).
               tokens.accent_soft.name(QColor::HexArgb), rule.name())
          .arg(kActiveMarkerWidth)
          .arg(kButtonWidth - 2 * kActiveMarkerWidth)
          .arg(kButtonHeight)
          .arg(kRailEdgeWidth)
          .arg(kBandGap));
  applying_style_sheet_ = false;
}

void ActivityBar::changeEvent(QEvent* event) {
  QToolBar::changeEvent(event);
  // Theme changes must apply live (docs/client/ux/README.md), and the marker
  // and rule are computed from the palette rather than read from it, so
  // nothing recomputes them for us.
  //
  // The guard is load-bearing, not defensive. The sheet is now set on the rail
  // itself rather than on a child, and `setStyleSheet` delivers a PaletteChange
  // to the widget it is set on — so without this the recompute re-enters
  // itself and recurses until the stack is gone. It crashed every run in the
  // constructor until 2026-08-30, before a single test body executed.
  if (applying_style_sheet_)
    return;
  if (event->type() == QEvent::PaletteChange ||
      event->type() == QEvent::ApplicationPaletteChange) {
    ApplyStyleSheet();
  }
}

QAction* ActivityBar::MakeAction(const QIcon& icon, const QString& tooltip) {
  QAction* action = addAction(icon, QString{});
  action->setCheckable(true);
  action->setToolTip(tooltip);
  return action;
}

QToolButton* ActivityBar::ButtonFor(const QAction* action) const {
  return qobject_cast<QToolButton*>(
      widgetForAction(const_cast<QAction*>(action)));
}

void ActivityBar::SetActiveMode(std::optional<PaneModeId> mode) {
  for (const Item& item : items_)
    item.action->setChecked(mode && item.mode.id == *mode);
}

void ActivityBar::SetModeAvailable(PaneModeId mode, bool available) {
  for (const Item& item : items_) {
    if (item.mode.id != mode)
      continue;
    item.action->setVisible(available);
    // The action drives the toolbar, but the widget only follows on the next
    // layout pass — which has not happened on a rail that is not yet shown. Set
    // it directly too, so hiding takes effect immediately either way.
    if (QToolButton* button = ButtonFor(item.action))
      button->setVisible(available);
    // A hidden button must not keep the marker: the rail would claim a mode
    // the operator cannot see.
    if (!available)
      item.action->setChecked(false);
    return;
  }
}

void ActivityBar::SetPageCallbacks(ActivatePageCallback on_activate_page,
                                   NewPageCallback on_new_page,
                                   PageContextMenuCallback on_page_context_menu,
                                   ReorderPageCallback on_reorder_page) {
  on_activate_page_ = std::move(on_activate_page);
  on_new_page_ = std::move(on_new_page);
  on_page_context_menu_ = std::move(on_page_context_menu);
  on_reorder_page_ = std::move(on_reorder_page);
}

int ActivityBar::PageDropIndexForY(int local_y) const {
  // Land before the first button whose midpoint is below the cursor; past the
  // last midpoint the page goes to the end.
  //
  // `local_y` is in rail coordinates but the buttons are children of the pages
  // band, so their own geometry is relative to that band. Map through instead
  // of reading geometry().y() directly — the two spaces differ by the band's
  // offset, which is exactly the kind of drift that makes a drop land one slot
  // out.
  int index = 0;
  for (const PageItem& item : page_items_) {
    const QToolButton* button = ButtonFor(item.action);
    if (!button)
      continue;
    const int midpoint =
        button->mapTo(this, QPoint{0, 0}).y() + button->height() / 2;
    if (local_y < midpoint)
      return index;
    ++index;
  }
  return index;
}

void ActivityBar::dragEnterEvent(QDragEnterEvent* event) {
  if (event->mimeData()->hasFormat(QLatin1String{kPageDragMimeType}))
    event->acceptProposedAction();
}

void ActivityBar::dragMoveEvent(QDragMoveEvent* event) {
  if (!event->mimeData()->hasFormat(QLatin1String{kPageDragMimeType}))
    return;
  event->acceptProposedAction();
  ShowDropIndicator(event->position().toPoint().y());
}

void ActivityBar::dragLeaveEvent(QDragLeaveEvent* event) {
  QToolBar::dragLeaveEvent(event);
  HideDropIndicator();
}

void ActivityBar::ShowDropIndicator(int local_y) {
  if (page_items_.empty())
    return;

  if (!drop_indicator_) {
    // A free-floating child positioned by geometry, deliberately NOT a widget
    // in the pages layout: inserting it there would shift the buttons, which
    // shifts the midpoints PageDropIndexForY reads, which moves the line — a
    // feedback loop that makes the indicator oscillate under a still cursor.
    drop_indicator_ = new QWidget{this};
    drop_indicator_->setObjectName(QStringLiteral("railDropIndicator"));
    drop_indicator_->setAutoFillBackground(true);
    drop_indicator_->setAttribute(Qt::WA_TransparentForMouseEvents);

    QPalette indicator_palette = drop_indicator_->palette();
    indicator_palette.setColor(QPalette::Window, RailTokens().accent);
    drop_indicator_->setPalette(indicator_palette);
  }

  // The boundary the page would land on: the top edge of the button now in
  // that slot, or the bottom edge of the last one when it would go to the end.
  const int index = PageDropIndexForY(local_y);
  const bool past_last = index >= static_cast<int>(page_items_.size());
  const QToolButton* anchor = ButtonFor(past_last ? page_items_.back().action
                                                  : page_items_[index].action);
  if (!anchor)
    return;
  const QPoint anchor_top_left = anchor->mapTo(this, QPoint{0, 0});
  const int y =
      past_last ? anchor_top_left.y() + anchor->height() : anchor_top_left.y();

  // Centred on the boundary rather than sitting below it, so the line reads as
  // the gap between two buttons and not as a rule belonging to one of them.
  drop_indicator_->setGeometry(anchor_top_left.x(),
                               y - kDropIndicatorHeight / 2, anchor->width(),
                               kDropIndicatorHeight);
  drop_indicator_->raise();
  drop_indicator_->show();
}

void ActivityBar::HideDropIndicator() {
  if (drop_indicator_)
    drop_indicator_->hide();
}

void ActivityBar::dropEvent(QDropEvent* event) {
  // Whatever the drop turns out to be, the line has done its job.
  HideDropIndicator();

  const QByteArray payload =
      event->mimeData()->data(QLatin1String{kPageDragMimeType});
  if (payload.isEmpty())
    return;

  const int page_id = payload.toInt();
  int new_index = PageDropIndexForY(event->position().toPoint().y());

  // Removing the dragged button shifts everything after it up by one, so a
  // drop below its old slot has to account for the gap it leaves behind.
  const auto dragged =
      std::ranges::find(page_items_, page_id,
                        [](const PageItem& item) { return item.page.page_id; });
  if (dragged != page_items_.end()) {
    const int old_index = static_cast<int>(dragged - page_items_.begin());
    if (new_index > old_index)
      --new_index;
  }

  event->acceptProposedAction();
  if (on_reorder_page_)
    on_reorder_page_(page_id, new_index);
}

bool ActivityBar::eventFilter(QObject* watched, QEvent* event) {
  auto* button = qobject_cast<QToolButton*>(watched);
  if (!button)
    return QToolBar::eventFilter(watched, event);

  const auto page = std::ranges::find_if(
      page_items_,
      [&](const PageItem& item) { return ButtonFor(item.action) == button; });
  if (page == page_items_.end())
    return QToolBar::eventFilter(watched, event);

  if (event->type() == QEvent::MouseButtonPress) {
    auto* mouse = static_cast<QMouseEvent*>(event);
    if (mouse->button() == Qt::LeftButton) {
      drag_press_pos_ = mouse->pos();
      drag_page_id_ = page->page.page_id;
    }
    return false;
  }

  if (event->type() == QEvent::MouseMove && drag_page_id_ != 0) {
    auto* mouse = static_cast<QMouseEvent*>(event);
    if (!(mouse->buttons() & Qt::LeftButton))
      return false;
    if ((mouse->pos() - drag_press_pos_).manhattanLength() <
        QApplication::startDragDistance()) {
      return false;
    }

    auto* mime = new QMimeData;
    mime->setData(QLatin1String{kPageDragMimeType},
                  QByteArray::number(drag_page_id_));
    auto* drag = new QDrag{button};
    drag->setMimeData(mime);
    drag->setPixmap(button->icon().pixmap(kIconSize, kIconSize));
    drag_page_id_ = 0;
    drag->exec(Qt::MoveAction);
    // The drag swallows the release, so the button would otherwise stay
    // visually pressed.
    button->setDown(false);
    return true;
  }

  if (event->type() == QEvent::MouseButtonRelease)
    drag_page_id_ = 0;

  return QToolBar::eventFilter(watched, event);
}

void ActivityBar::SetPages(std::vector<PageButton> pages) {
  for (const PageItem& item : page_items_) {
    // Detach the button before deferring its deletion. `removeAction` drops
    // the action but leaves the button the toolbar built for it parented until
    // the event loop runs, and until then it is still a child, still painted,
    // and still found by findChildren — measured 2026-08-30, a rebuild from
    // three pages to four left seven buttons on the rail. Deleting
    // synchronously is not an option: SetPages can run from a page button's
    // own handler (activate -> OpenPage -> refresh).
    QToolButton* button = ButtonFor(item.action);
    removeAction(item.action);
    if (button) {
      button->setParent(nullptr);
      button->deleteLater();
    }
    item.action->deleteLater();
  }
  page_items_.clear();

  const scada::aui::ThemeTokens& tokens = RailTokens();

  for (std::size_t index = 0; index < pages.size(); ++index) {
    const PageButton& page = pages[index];
    // The ordinal is by rail position, not by page id: the operator counts
    // buttons, and profile ids have gaps once pages are deleted.
    const QString ordinal = QString::number(index + 1);
    // Icon first, ordinal as the fallback. An unknown key lands here too, so a
    // profile from a newer build degrades to the old numbered button rather
    // than to a blank one.
    const std::string_view glyph = PageGlyph(page.icon_key);
    QIcon icon = glyph.empty()
                     ? TextIcon(ordinal, tokens.fg_on_dark, kIconSize)
                     : LoadTintedGlyph(glyph, kIconSize, tokens.fg_on_dark,
                                       qApp ? qApp->devicePixelRatio() : 1.0);

    // `2 · Alarms` — the ordinal names the Ctrl+N shortcut and the title says
    // what the page holds, neither of which fits on the button itself.
    const QString label =
        ordinal + QStringLiteral(" · ") + QString::fromStdU16String(page.title);

    // Inserted before the "+", so the pages stay between the separator and the
    // "+" however often this is rebuilt.
    auto* action = new QAction{icon, QString{}, this};
    action->setCheckable(true);
    action->setToolTip(label);
    if (page.opened_elsewhere) {
      action->setEnabled(false);
      action->setToolTip(
          label + QStringLiteral(" — ") +
          QString::fromStdU16String(Translate("open in another window")));
    }
    insertAction(new_page_action_, action);

    const int page_id = page.page_id;
    connect(action, &QAction::triggered, this, [this, page_id] {
      if (on_activate_page_)
        on_activate_page_(page_id);
    });

    if (QToolButton* button = ButtonFor(action)) {
      button->setContextMenuPolicy(Qt::CustomContextMenu);
      button->installEventFilter(this);
      connect(button, &QToolButton::customContextMenuRequested, this,
              [this, button, page_id](const QPoint& pos) {
                if (on_page_context_menu_)
                  on_page_context_menu_(page_id, button->mapToGlobal(pos));
              });
    }

    page_items_.emplace_back(PageItem{page, action});
  }

  // Re-assert the marker: the actions it referred to were just destroyed.
  SetActivePage(active_page_id_);
}

void ActivityBar::SetActivePage(int page_id) {
  active_page_id_ = page_id;
  for (const PageItem& item : page_items_)
    item.action->setChecked(item.page.page_id == page_id);
}

void ActivityBar::SetUtilities(std::vector<Utility> utilities,
                               ActivateUtilityCallback on_activate_utility) {
  on_activate_utility_ = std::move(on_activate_utility);

  const scada::aui::ThemeTokens& tokens = RailTokens();

  // Appended after the expanding spacer the constructor added, which is what
  // pins the group to the foot however tall the pages group grows.
  for (const Utility& utility : utilities) {
    QAction* action = MakeAction(
        ModeIcon(Mode{.label = utility.label, .icon_kind = utility.icon_kind},
                 tokens.fg_on_dark, kIconSize),
        QString::fromStdU16String(utility.label));

    const int utility_id = utility.utility_id;
    connect(action, &QAction::triggered, this, [this, utility_id] {
      if (on_activate_utility_)
        on_activate_utility_(utility_id);
    });

    utility_items_.emplace_back(UtilityItem{utility, action});
  }
}

void ActivityBar::SetUtilityAvailable(int utility_id, bool available) {
  for (const UtilityItem& item : utility_items_) {
    if (item.utility.utility_id != utility_id)
      continue;
    item.action->setVisible(available);
    if (QToolButton* button = ButtonFor(item.action))
      button->setVisible(available);
    // Same rule as a hidden mode: the rail must not claim a surface the
    // operator cannot see.
    if (!available)
      item.action->setChecked(false);
    return;
  }
}

void ActivityBar::SetActiveUtility(std::optional<int> utility_id) {
  for (const UtilityItem& item : utility_items_) {
    item.action->setChecked(utility_id &&
                            item.utility.utility_id == *utility_id);
  }
}
