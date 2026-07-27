#include "main_window/activity_bar_qt.h"

#include "aui/qt/theme_qt.h"
#include "aui/severity_colors.h"
#include "aui/translation.h"

#include <QApplication>
#include <QDrag>
#include <QDragEnterEvent>
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

#include <algorithm>
#include <ranges>

namespace {

// The drag payload for page reordering. A private MIME type, so a drag from
// somewhere else in the app can never be mistaken for a page.
constexpr char kPageDragMimeType[] = "application/x-scada-rail-page";

constexpr int kRailWidth = 52;
constexpr int kButtonSize = 44;
constexpr int kIconSize = 24;

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

// Draws the dedicated line glyph for a section within `r` (a square icon box)
// using the already-configured pen. Simple 2px vector marks, so they stay crisp
// on the charcoal rail at any DPI without shipping raster assets.
void DrawModeGlyph(QPainter& p, ActivityBar::Icon kind, const QRectF& r) {
  const qreal x = r.x(), y = r.y(), w = r.width(), h = r.height();
  switch (kind) {
    case ActivityBar::Icon::kObjects: {
      // Tree: a root node with a spine dropping to two children. The child
      // nodes are drawn as filled dots — three bare horizontal branches off a
      // vertical spine render as a capital "E" at 24 px.
      const qreal spine_x = x + w * 0.26;
      const qreal dot = w * 0.09;
      const qreal root_y = y + h * 0.2;
      const qreal mid_y = y + h * 0.52;
      const qreal low_y = y + h * 0.8;

      p.drawEllipse(QPointF(spine_x, root_y), dot, dot);
      p.drawLine(QPointF(spine_x, root_y + dot), QPointF(spine_x, low_y));
      p.drawLine(QPointF(spine_x, mid_y), QPointF(x + w * 0.6, mid_y));
      p.drawLine(QPointF(spine_x, low_y), QPointF(x + w * 0.6, low_y));
      p.drawEllipse(QPointF(x + w * 0.68, mid_y), dot, dot);
      p.drawEllipse(QPointF(x + w * 0.68, low_y), dot, dot);
      break;
    }
    case ActivityBar::Icon::kDevices: {
      // Schematic: top busbar feeding a node (transformer-ish circle).
      p.drawLine(QPointF(x + w * 0.15, y + h * 0.22),
                 QPointF(x + w * 0.85, y + h * 0.22));
      p.drawLine(QPointF(x + w * 0.5, y + h * 0.22),
                 QPointF(x + w * 0.5, y + h * 0.45));
      p.drawEllipse(QRectF(x + w * 0.32, y + h * 0.45, w * 0.36, h * 0.36));
      break;
    }
    case ActivityBar::Icon::kFiles: {
      // Folder: tab along the top edge, then the body.
      QPainterPath folder;
      folder.moveTo(x + w * 0.14, y + h * 0.76);
      folder.lineTo(x + w * 0.14, y + h * 0.26);
      folder.lineTo(x + w * 0.42, y + h * 0.26);
      folder.lineTo(x + w * 0.5, y + h * 0.38);
      folder.lineTo(x + w * 0.86, y + h * 0.38);
      folder.lineTo(x + w * 0.86, y + h * 0.76);
      folder.closeSubpath();
      p.drawPath(folder);
      break;
    }
    case ActivityBar::Icon::kNodes: {
      // Node graph: three vertices joined by two edges. Deliberately distinct
      // from kDevices, which has a single circle under a busbar.
      const QPointF top{x + w * 0.5, y + h * 0.22};
      const QPointF left{x + w * 0.22, y + h * 0.74};
      const QPointF right{x + w * 0.78, y + h * 0.74};
      p.drawLine(top, left);
      p.drawLine(top, right);
      const qreal radius = w * 0.11;
      p.drawEllipse(top, radius, radius);
      p.drawEllipse(left, radius, radius);
      p.drawEllipse(right, radius, radius);
      break;
    }
    case ActivityBar::Icon::kNewPage: {
      // Plus sign — "add a page", matching the web client's new-tab affordance.
      p.drawLine(QPointF(x + w * 0.5, y + h * 0.22),
                 QPointF(x + w * 0.5, y + h * 0.78));
      p.drawLine(QPointF(x + w * 0.22, y + h * 0.5),
                 QPointF(x + w * 0.78, y + h * 0.5));
      break;
    }
    case ActivityBar::Icon::kNone:
      break;
  }
}

// A rail button icon that always shows something: the dedicated section glyph,
// or a charcoal-friendly glyph of the label's first letter when the section has
// no dedicated icon.
QIcon ModeIcon(const ActivityBar::Mode& mode, const QColor& fg) {
  QPixmap pixmap{kIconSize, kIconSize};
  pixmap.fill(Qt::transparent);
  QPainter painter{&pixmap};
  painter.setRenderHint(QPainter::Antialiasing);

  if (mode.icon_kind != ActivityBar::Icon::kNone) {
    QPen pen{fg};
    pen.setWidthF(1.8);
    pen.setJoinStyle(Qt::RoundJoin);
    pen.setCapStyle(Qt::RoundCap);
    painter.setPen(pen);
    painter.setBrush(Qt::NoBrush);
    // Inset so the 2px stroke stays inside the box.
    DrawModeGlyph(painter, mode.icon_kind,
                  QRectF(2, 2, kIconSize - 4, kIconSize - 4));
    return QIcon{pixmap};
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
               tokens.accent.name(), tokens.accent_soft.name()));

  auto* layout = new QVBoxLayout{this};
  layout->setContentsMargins(0, 6, 0, 6);
  layout->setSpacing(2);

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

  // Divider between the two groups: pane modes select what fills the sidebar,
  // pages replace the whole workspace. Different granularity, so they read as
  // separate groups rather than one undifferentiated column.
  pages_divider_ = new QWidget{this};
  pages_divider_->setObjectName(QStringLiteral("railDivider"));
  pages_divider_->setFixedHeight(1);
  pages_divider_->setFixedWidth(kRailWidth - 16);
  layout->addSpacing(6);
  layout->addWidget(pages_divider_, 0, Qt::AlignHCenter);
  layout->addSpacing(6);

  pages_layout_ = new QVBoxLayout;
  pages_layout_->setContentsMargins(0, 0, 0, 0);
  pages_layout_->setSpacing(2);
  layout->addLayout(pages_layout_);

  new_page_button_ =
      MakeButton(TextIcon(QStringLiteral("+"), tokens.fg_on_dark),
                 QString::fromStdU16String(Translate("New page")));
  // The "+" is an action, not a destination — it must never carry a marker.
  new_page_button_->setCheckable(false);
  connect(new_page_button_, &QToolButton::clicked, this, [this] {
    if (on_new_page_)
      on_new_page_();
  });
  layout->addWidget(new_page_button_, 0, Qt::AlignHCenter);

  layout->addStretch(1);

  SetPages({});
}

ActivityBar::~ActivityBar() = default;

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
  int index = 0;
  for (const PageItem& item : page_items_) {
    const int midpoint =
        item.button->geometry().y() + item.button->geometry().height() / 2;
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
  if (event->mimeData()->hasFormat(QLatin1String{kPageDragMimeType}))
    event->acceptProposedAction();
}

void ActivityBar::dropEvent(QDropEvent* event) {
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
    // Numbered by rail position, not by page id: the operator counts buttons,
    // and profile ids have gaps once pages are deleted.
    QToolButton* button =
        MakeButton(TextIcon(QString::number(index + 1), tokens.fg_on_dark),
                   QString::fromStdU16String(page.title));
    if (page.opened_elsewhere) {
      button->setEnabled(false);
      button->setToolTip(
          QString::fromStdU16String(page.title) + QStringLiteral(" — ") +
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
