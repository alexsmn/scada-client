#include "main_window/context_bar_qt.h"

#include "aui/translation.h"
#include "main_window/alarm_state_cluster_qt.h"
#include "main_window/breadcrumb_qt.h"
#include "main_window/command_field_qt.h"

#include <QHBoxLayout>
#include <QStyle>

#include <utility>

ContextBar::ContextBar(AlarmStateCluster::CountsProvider alarm_counts,
                       OpenPaletteCallback open_palette,
                       QWidget* parent)
    : QToolBar{parent}, open_palette_{std::move(open_palette)} {
  setObjectName(QStringLiteral("ContextBar"));
  setMovable(false);
  setFloatable(false);
  setContextMenuPolicy(Qt::PreventContextMenu);

  // Give the bar vertical breathing room, taken from the style.
  //
  // A QToolBar states no height of its own: it collapses to its tallest child
  // plus the style's frame. All three slots zeroed their vertical margins and
  // CommandField::sizeHint() forwards QLineEdit's height unchanged, so the
  // field alone decided the bar and the bar hugged it -- measured at 29px with
  // 3px of clearance around a 23px field.
  //
  // The padding is a style metric and never a literal, so it tracks DPI and the
  // OS text size (docs/client/ux/README.md: "Metrics come from the platform ...
  // Hard-coded pixel sizes are defects"). Deliberately NOT setFixedHeight: that
  // would pin the bar to one machine's pixels and break text-size
  // accessibility, and matching the mockups' 40px is explicitly not the goal --
  // docs/product/ui-mockups/mockup-caveats.md, "What is not a divergence",
  // exempts Qt control metrics from the HTML screens.
  //
  // It goes on the three SLOTS and not on the toolbar's own layout, which was
  // tried first and is wrong in a way only a render shows: QToolBarLayout hands
  // a vertically-Fixed child its sizeHint height and top-aligns it, so padding
  // the toolbar grew the bar to 41px while leaving the field at the top with
  // 3px above it and 15px below. Padding each slot instead makes the slot the
  // tallest child, and a QHBoxLayout centres a Fixed-height widget inside it.
  const int slot_padding =
      style()->pixelMetric(QStyle::PM_LayoutVerticalSpacing, nullptr, this);

  // And horizontally, for the same reason and from the same place. A QToolBar
  // contributes only its style frame on the left and right -- measured at 4px
  // and 7px on macOS -- so with every slot zeroing its horizontal margins the
  // breadcrumb's first glyph sat hard against the window edge and the last
  // alarm tile against the other. `operator-shell.html` gives `.topbar` a
  // `padding: 0 12px` for exactly this, and the two bars either side of this
  // one already stand off the edge (the menu bar by its style, the status strip
  // by its cells' own margins), so the context bar was the one row in the
  // window whose text started at x=4.
  //
  // The inset goes on the two OUTER slots only. The centre slot is bounded by
  // its neighbours rather than by the window, and padding it would narrow the
  // command field for no reason.
  const int bar_margin_left =
      style()->pixelMetric(QStyle::PM_LayoutLeftMargin, nullptr, this);
  const int bar_margin_right =
      style()->pixelMetric(QStyle::PM_LayoutRightMargin, nullptr, this);

  // Three slots, as `operator-shell.html` lays the bar out: breadcrumb left,
  // command field centre, alarm state right. Each side is a container of its
  // own rather than "content plus a spacer" — the spacers this replaced sat
  // *between* the content, so the field's position was a function of what the
  // two sides happened to hold rather than of the bar. Which slot a widget
  // belongs to is now stated by the code, not implied by insertion order.
  //
  // Note this makes the field's placement architecturally right without making
  // it pixel-centred: the columns start from their children's size hints, so a
  // wide alarm cluster still shifts the field. Exact centring is the mockup's
  // own idiom (equal `1fr` grid columns) and appearance is the platform's here
  // — see client/CLAUDE.md, "the mockups are not a visual target".
  auto* left_slot = new QWidget(this);
  left_slot->setObjectName(QStringLiteral("contextBarLeftSlot"));
  auto* left_layout = new QHBoxLayout(left_slot);
  left_layout->setContentsMargins(bar_margin_left, slot_padding, 0,
                                  slot_padding);
  left_slot->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

  // No brand lockup. A native application identifies itself in the window
  // title and the About dialog, not with an in-window mark — and the one that
  // stood here was a hard-coded, space-padded, untranslatable literal
  // (docs/client/ux/shell.md §2.2, native rework).
  // Where the workspace is: page → view → selected object. The left slot the
  // brand lockup vacated, which is the slot `config-workbench.html` draws a
  // breadcrumb in. It coexists with the command field rather than replacing it
  // — shell.md §2.2 — because the two answer different questions.
  breadcrumb_ = new Breadcrumb(left_slot);
  left_layout->addWidget(breadcrumb_);
  left_layout->addStretch();
  addWidget(left_slot);

  // Command/search entry point (centre). It owns no text: clicking it, or
  // typing into it, opens the command palette, which is where the typing
  // happens. The field draws the magnifier and the shortcut hint itself.
  command_field_ = new CommandField(
      this,
      QString::fromStdU16String(Translate("Search tags, objects, commands…")),
      PaletteKey(),
      [this](const QString& initial_text) { open_palette_(initial_text); });
  // The centre slot is a container like the two beside it, so the field takes
  // the same padding and is centred in it rather than sitting against the
  // bar's top edge. Preferred (not Expanding) horizontally, so the two
  // Expanding side slots still centre it against the bar.
  auto* centre_slot = new QWidget(this);
  centre_slot->setObjectName(QStringLiteral("contextBarCentreSlot"));
  auto* centre_layout = new QHBoxLayout(centre_slot);
  centre_layout->setContentsMargins(0, slot_padding, 0, slot_padding);
  centre_slot->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
  centre_layout->addWidget(command_field_);
  addWidget(centre_slot);

  // Right slot: alarm state alone -- the escalation chips, then the
  // per-severity tiles. See AlarmStateCluster. The counts come from the host,
  // which owns the model they are read from.
  alarm_state_ = new AlarmStateCluster(std::move(alarm_counts), this);
  alarm_state_->setObjectName(QStringLiteral("contextBarRightSlot"));
  alarm_state_->layout()->setContentsMargins(0, slot_padding, bar_margin_right,
                                             slot_padding);
  alarm_state_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
  addWidget(alarm_state_);

  // No identity/connection cluster here. Who/where context (user·role,
  // connection, server latency, endpoint·build) is stated once, in the status
  // strip — mirroring it in the top bar duplicated four cells verbatim and
  // costs a place to look without adding a fact (shell.md §3: don't show the
  // same metric in two prominent places). The top bar carries only alarm state,
  // which needs pre-attentive prominence.
}

ContextBar::~ContextBar() = default;

// static
QKeySequence ContextBar::PaletteKey() {
  return QKeySequence{Qt::CTRL | Qt::Key_K};
}

void ContextBar::SetBreadcrumb(std::span<const Breadcrumb::Segment> segments) {
  breadcrumb_->SetSegments(segments);
}

void ContextBar::RefreshAlarmState() {
  alarm_state_->Refresh();
}
