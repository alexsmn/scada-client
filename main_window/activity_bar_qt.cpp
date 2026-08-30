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
#include <QIcon>
#include <QMimeData>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPen>

#include <QPixmap>
#include <QToolButton>
#include <QVBoxLayout>
#include <string_view>

#include <algorithm>
#include <ranges>

namespace {

// The drag payload for page reordering. A private MIME type, so a drag from
// somewhere else in the app can never be mistaken for a page.
constexpr char kPageDragMimeType[] = "application/x-scada-rail-page";

constexpr int kRailWidth = 52;
// Inset of the page buttons inside their band. Enough that the band reads as a
// container around them rather than as a stripe behind them.
constexpr int kBandPadding = 3;
constexpr int kButtonSize = 44;
constexpr int kIconSize = 24;
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
QIcon ModeIcon(const ActivityBar::Mode& mode, const QColor& fg) {
  QPixmap pixmap{kIconSize, kIconSize};
  pixmap.fill(Qt::transparent);
  QPainter painter{&pixmap};
  painter.setRenderHint(QPainter::Antialiasing);

  if (const std::string_view glyph = ModeGlyph(mode.icon_kind);
      !glyph.empty()) {
    painter.end();
    return LoadTintedGlyph(glyph, kIconSize, fg,
                           qApp ? qApp->devicePixelRatio() : 1.0);
  }

  painter.setPen(fg);
  QFont font = painter.font();
  font.setPixelSize(kIconSize - 6);
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
// whose titles are arbitrary and cannot fit a 52 px rail.
QIcon TextIcon(const QString& text, const QColor& fg) {
  QPixmap pixmap{kIconSize, kIconSize};
  pixmap.fill(Qt::transparent);
  QPainter painter{&pixmap};
  painter.setRenderHint(QPainter::Antialiasing);
  painter.setPen(fg);
  QFont font = painter.font();
  font.setPixelSize(kIconSize - 8);
  font.setBold(true);
  painter.setFont(font);
  painter.drawText(pixmap.rect(), Qt::AlignCenter, text);
  return QIcon{pixmap};
}

}  // namespace

ActivityBar::ActivityBar(QWidget* parent,
                         std::vector<Mode> modes,
                         ActivateCallback on_activate)
    : QWidget{parent}, on_activate_{std::move(on_activate)} {
  setObjectName(QStringLiteral("activityBar"));
  setFixedWidth(kRailWidth);
  setAcceptDrops(true);

  const scada::aui::ThemeTokens& tokens = RailTokens();
  // Charcoal rail with token-driven active/hover states; the checked (active)
  // button gets an accent left-marker and a soft accent fill.
  setStyleSheet(
      QStringLiteral(
          "#activityBar { background: %1; }"
          "#activityBar QToolButton { border: none; border-left: 3px solid "
          "transparent; background: transparent; }"
          "#activityBar QToolButton:hover { background: %2; }"
          "#activityBar QToolButton:checked { border-left: 3px solid %3; "
          "background: %4; }"
          "#railDivider { background: %2; }")
          .arg(tokens.rail_bg.name(), tokens.surface_muted.name(),
               tokens.accent.name(),
               // HexArgb, because `accent_soft` carries an alpha (.15 dark,
               // .10 light, and a derived one under the system palette) that
               // the default #RRGGBB name() drops. Dropped, the active marker
               // renders as a solid accent block at ~6.7x its intended
               // strength — the loudest thing in the rail, where the design
               // asks for a tint under the accent edge. Every other consumer
               // of this token already names it this way (`aui/qt/grid.cpp`,
               // `modules/table/qt/table_toolbar.cpp`,
               // `modules/events/qt/area_sidebar.cpp`).
               tokens.accent_soft.name(QColor::HexArgb)));

  auto* layout = new QVBoxLayout{this};
  layout->setContentsMargins(0, 6, 0, 6);
  layout->setSpacing(2);
  root_layout_ = layout;

  for (const Mode& mode : modes) {
    QToolButton* button = MakeButton(ModeIcon(mode, tokens.fg_on_dark),
                                     QString::fromStdU16String(mode.label));
    layout->addWidget(button, 0, Qt::AlignHCenter);

    const PaneModeId id = mode.id;
    items_.emplace_back(Item{mode, button});
    connect(button, &QToolButton::clicked, this, [this, id] {
      if (on_activate_)
        on_activate_(id);
    });
  }

  // The pages group sits on its own band. A pane mode and a page are active at
  // the same time and the two markers are drawn identically, so something has
  // to say which is which; a plain divider left them reading as one column
  // with two selections. The band groups the pages instead — same marker, but
  // it lands inside a container that is visibly the page group.
  pages_band_ = new QWidget{this};
  pages_band_->setObjectName(QStringLiteral("railPagesBand"));
  pages_band_->setAutoFillBackground(true);

  auto* band_layout = new QVBoxLayout{pages_band_};
  band_layout->setContentsMargins(kBandPadding, kBandPadding, kBandPadding,
                                  kBandPadding);
  band_layout->setSpacing(2);

  pages_layout_ = new QVBoxLayout;
  pages_layout_->setContentsMargins(0, 0, 0, 0);
  pages_layout_->setSpacing(2);
  band_layout->addLayout(pages_layout_);

  // The Lucide `plus`, not the literal character: the rail reads from one
  // icon set (docs/client/ux/iconography.md §5.3, which has keyed `kNewPage`
  // to that glyph since the set landed), and a drawn `+` sat at a different
  // weight and on a different grid from every button above it. The web
  // client's rail draws the same stroke plus, and so does
  // activity-rail.html's `.ic.add`.
  new_page_button_ =
      MakeButton(ModeIcon(Mode{.label = u"+", .icon_kind = Icon::kNewPage},
                          tokens.fg_on_dark),
                 QString::fromStdU16String(Translate("New page")));
  // The "+" is an action, not a destination — it must never carry a marker.
  new_page_button_->setCheckable(false);
  connect(new_page_button_, &QToolButton::clicked, this, [this] {
    if (on_new_page_)
      on_new_page_();
  });
  band_layout->addWidget(new_page_button_, 0, Qt::AlignHCenter);

  layout->addSpacing(6);
  layout->addWidget(pages_band_, 0, Qt::AlignHCenter);
  layout->addSpacing(6);

  ApplyBandPalette();

  layout->addStretch(1);

  SetPages({});
}

ActivityBar::~ActivityBar() = default;

void ActivityBar::ApplyBandPalette() {
  if (!pages_band_)
    return;

  // Derived from the rail's own window colour rather than a baked value, so the
  // band follows the platform palette and the OS light/dark preference without
  // a stylesheet: lighter on a dark palette, darker on a light one. Deriving it
  // also means it stays correct under the high-contrast palette, where a fixed
  // tint would either vanish or shout.
  const QColor base = palette().color(QPalette::Window);
  const QColor band =
      base.lightness() < 128 ? base.lighter(128) : base.darker(107);

  QPalette band_palette = pages_band_->palette();
  band_palette.setColor(QPalette::Window, band);
  pages_band_->setPalette(band_palette);
}

void ActivityBar::changeEvent(QEvent* event) {
  QWidget::changeEvent(event);
  // Theme changes must apply live (docs/client/ux/README.md), and the band is
  // computed from the palette rather than read from it, so nothing recomputes
  // it for us.
  if (event->type() == QEvent::PaletteChange ||
      event->type() == QEvent::ApplicationPaletteChange) {
    ApplyBandPalette();
  }
}

QToolButton* ActivityBar::MakeButton(const QIcon& icon,
                                     const QString& tooltip) {
  auto* button = new QToolButton{this};
  button->setCheckable(true);
  button->setAutoRaise(true);
  button->setFixedSize(kButtonSize, kButtonSize);
  button->setIconSize({kIconSize, kIconSize});
  button->setIcon(icon);
  button->setToolTip(tooltip);
  return button;
}

void ActivityBar::SetActiveMode(std::optional<PaneModeId> mode) {
  for (const Item& item : items_)
    item.button->setChecked(mode && item.mode.id == *mode);
}

void ActivityBar::SetModeAvailable(PaneModeId mode, bool available) {
  for (const Item& item : items_) {
    if (item.mode.id != mode)
      continue;
    item.button->setVisible(available);
    // A hidden button must not keep the marker: the rail would claim a mode
    // the operator cannot see.
    if (!available)
      item.button->setChecked(false);
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
    const int midpoint =
        item.button->mapTo(this, QPoint{0, 0}).y() + item.button->height() / 2;
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
  QWidget::dragLeaveEvent(event);
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
  const QToolButton* anchor =
      past_last ? page_items_.back().button : page_items_[index].button;
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
    return QWidget::eventFilter(watched, event);

  const auto page = std::ranges::find(
      page_items_, button, [](const PageItem& item) { return item.button; });
  if (page == page_items_.end())
    return QWidget::eventFilter(watched, event);

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

  return QWidget::eventFilter(watched, event);
}

void ActivityBar::SetPages(std::vector<PageButton> pages) {
  for (const PageItem& item : page_items_) {
    pages_layout_->removeWidget(item.button);
    // Reparent before deleteLater(): SetPages can run from a page button's own
    // clicked handler (activate -> OpenPage -> refresh), so the widget cannot
    // be deleted synchronously — but until the deferred delete runs it would
    // still be a child of the rail, still painted, and still found by
    // findChildren. Detaching now makes the rebuild take effect immediately.
    item.button->setParent(nullptr);
    item.button->deleteLater();
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
                     ? TextIcon(ordinal, tokens.fg_on_dark)
                     : LoadTintedGlyph(glyph, kIconSize, tokens.fg_on_dark,
                                       qApp ? qApp->devicePixelRatio() : 1.0);

    // `2 · Alarms` — the ordinal names the Ctrl+N shortcut and the title says
    // what the page holds, neither of which fits on the button itself.
    const QString label =
        ordinal + QStringLiteral(" · ") + QString::fromStdU16String(page.title);
    QToolButton* button = MakeButton(icon, label);
    if (page.opened_elsewhere) {
      button->setEnabled(false);
      button->setToolTip(
          label + QStringLiteral(" — ") +
          QString::fromStdU16String(Translate("open in another window")));
    }
    button->setContextMenuPolicy(Qt::CustomContextMenu);
    button->installEventFilter(this);

    const int page_id = page.page_id;
    connect(button, &QToolButton::clicked, this, [this, page_id] {
      if (on_activate_page_)
        on_activate_page_(page_id);
    });
    connect(button, &QToolButton::customContextMenuRequested, this,
            [this, button, page_id](const QPoint& pos) {
              if (on_page_context_menu_)
                on_page_context_menu_(page_id, button->mapToGlobal(pos));
            });

    pages_layout_->addWidget(button, 0, Qt::AlignHCenter);
    page_items_.emplace_back(PageItem{page, button});
  }

  // Re-assert the marker: the buttons it referred to were just destroyed.
  SetActivePage(active_page_id_);
}

void ActivityBar::SetActivePage(int page_id) {
  active_page_id_ = page_id;
  for (const PageItem& item : page_items_)
    item.button->setChecked(item.page.page_id == page_id);
}

void ActivityBar::SetUtilities(std::vector<Utility> utilities,
                               ActivateUtilityCallback on_activate_utility) {
  on_activate_utility_ = std::move(on_activate_utility);

  const scada::aui::ThemeTokens& tokens = RailTokens();

  // Appended after the stretch the constructor added, which is what pins the
  // group to the foot however tall the pages group grows.
  for (const Utility& utility : utilities) {
    QToolButton* button = MakeButton(
        ModeIcon(Mode{.label = utility.label, .icon_kind = utility.icon_kind},
                 tokens.fg_on_dark),
        QString::fromStdU16String(utility.label));
    root_layout_->addWidget(button, 0, Qt::AlignHCenter);

    const int utility_id = utility.utility_id;
    connect(button, &QToolButton::clicked, this, [this, utility_id] {
      if (on_activate_utility_)
        on_activate_utility_(utility_id);
    });

    utility_items_.emplace_back(UtilityItem{utility, button});
  }
}

void ActivityBar::SetUtilityAvailable(int utility_id, bool available) {
  for (const UtilityItem& item : utility_items_) {
    if (item.utility.utility_id != utility_id)
      continue;
    item.button->setVisible(available);
    // Same rule as a hidden mode: the rail must not claim a surface the
    // operator cannot see.
    if (!available)
      item.button->setChecked(false);
    return;
  }
}

void ActivityBar::SetActiveUtility(std::optional<int> utility_id) {
  for (const UtilityItem& item : utility_items_) {
    item.button->setChecked(utility_id &&
                            item.utility.utility_id == *utility_id);
  }
}
