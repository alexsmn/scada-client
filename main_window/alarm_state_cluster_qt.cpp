#include "main_window/alarm_state_cluster_qt.h"

#include "aui/qt/theme_qt.h"
#include "aui/severity_colors.h"
#include "aui/translation.h"
#include "base/blinker.h"
#include "events/alarm_escalation.h"
#include "events/qt/severity_tile_strip.h"

#include <QFontMetrics>
#include <QHBoxLayout>
#include <QLabel>
#include <QTimer>

#include <optional>
#include <utility>

namespace {

// The annunciator's flash half-period. Slow enough to read the caption
// through, fast enough to be pre-attentive; ISA-18.2 asks for a flash rate in
// this region for an unacknowledged alarm.
constexpr scada::Duration kAnnunciatorFlashHalfPeriod =
    std::chrono::milliseconds{700};

// How often the phase is SAMPLED, which is not the same as how often it flips.
// Half the half-period, for the reason `kBlinkTick` gives in blinker.cpp: at
// exactly one flip per tick the sampling aliases and the flash can stall or
// double up.
constexpr int kAnnunciatorFlashInterval = 350;

// Corner radius for the escalation chips, which are pills: half the chip's own
// height -- exactly, because the chips are vertically Fixed -- so the shape
// survives any OS text size. This was a hard-coded 9px,
// which is a defect of the kind docs/client/ux/README.md names outright -- a
// chip whose height is font-derived but whose radius is not stops reading as a
// pill the moment the font grows past twice that radius, which is exactly what
// an accessibility text-size bump does.
//
// `border` is the stylesheet border width the caller writes in the same rule: a
// QSS border widens the label past the font-and-margin box QLabel sizes itself
// to, and a radius that ignored it would fall short of half the drawn height.
int PillRadius(const QLabel& label, int border) {
  const QFontMetrics metrics{label.font()};
  return (metrics.height() + 2 * label.margin() + 2 * border) / 2;
}

// The critical severity fill, or the dark theme's value if none is installed.
QColor CriticalFill() {
  const std::optional<scada::aui::Color> color =
      scada::aui::SeverityColor(scada::aui::SeverityLevel::kCritical);
  return color ? color->qcolor() : QColor{0xe8, 0x5a, 0x52};
}

// An escalation chip: hidden until its rung lights.
QLabel* MakeChip(QWidget* parent) {
  auto* chip = new QLabel(parent);
  chip->setMargin(2);
  // Fixed vertically so the chip is its own natural height and is
  // centred in the slot. Left to stretch, it fills the row and the
  // font-derived PillRadius then falls short of half the drawn height
  // -- measured at a 46px chip taking a 17px radius under a 2x font,
  // which reads as a rounded rectangle rather than a pill.
  chip->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
  chip->setVisible(false);
  return chip;
}

}  // namespace

AlarmStateCluster::AlarmStateCluster(CountsProvider counts, QWidget* parent)
    : QWidget{parent}, counts_{std::move(counts)} {
  auto* layout = new QHBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->addStretch();

  // Rung 1 — the ISA-18.2 annunciator: any unacknowledged critical alarm.
  // Below the flood threshold this is the *only* thing that marks a critical
  // alarm nobody has taken; before it existed, one such alarm read as a tile
  // going from 0 to 1 and nothing else.
  //
  // The flash is NOT gated on any reduce-motion preference, and that is a
  // decision rather than something nobody got to (2026-08-31). An ISA-18.2
  // annunciation is a safety signal, so it must not be suppressible by a
  // setting the operator chose for their desktop and the plant never agreed
  // to. Qt exposes no reduce-motion query to consult in any case -- checked
  // against the Qt this tree vendors, 6.11.1: QStyleHints declares no such
  // property and no Qt6 header mentions one -- but the point is that one
  // would not be consulted here if it did.
  //
  // WCAG 2.2.2 Pause, Stop, Hide is the criterion that would apply on a web
  // surface, and its exception covers movement "part of an activity where it
  // is essential"
  // (https://www.w3.org/WAI/WCAG22/Understanding/pause-stop-hide.html,
  // verified 2026-08-31). The flash rate is well under 2.3.1's three-per-
  // second threshold. If the motion ever
  // needs softening, soften it for everybody -- never switch the annunciator
  // off for the operators most likely to be sitting in front of it all shift.
  annunciator_ = MakeChip(this);
  layout->addWidget(annunciator_);

  // Flashing is part of the signal, not decoration (ISA-18.2): a standing
  // unacknowledged critical must keep asserting itself. The timer runs only
  // while the rung is lit, and the two phases differ in fill rather than in
  // presence — a chip that blinks out entirely can be missed in the dark half
  // of its own cycle.
  annunciator_flash_ = new QTimer(this);
  annunciator_flash_->setInterval(kAnnunciatorFlashInterval);
  connect(annunciator_flash_, &QTimer::timeout, this, [this] {
    // Sampled from the clock, never toggled. A free-running toggle advances
    // with the event loop, so a frozen clock cannot hold it still and the
    // screenshot generator caught this chip mid-flash — lit in one render,
    // outlined in the next, from one unchanged binary (visual_review V54).
    // blinker.h explains the rule; this widget predated anyone applying it.
    const bool on = BlinkPhaseAt(scada::Now(), kAnnunciatorFlashHalfPeriod);
    if (on == annunciator_flash_on_)
      return;
    annunciator_flash_on_ = on;
    StyleAnnunciator();
  });

  // Rung 2 — alarm-flood escalation pill, left of the per-severity tiles so it
  // reads as the dominant state during a flood. A flood outranks a single
  // critical, so it is drawn solid where the annunciator is drawn as an
  // outline.
  flood_pill_ = MakeChip(this);
  layout->addWidget(flood_pill_);

  // Live severity KPI tiles (backlog 2.3): critical / warning / unacknowledged,
  // ordered and coloured by the shared tile builder.
  severity_tiles_ = events::MakeSeverityTileStrip(counts_, this);
  if (severity_tiles_)
    layout->addWidget(severity_tiles_);

  Refresh();
}

AlarmStateCluster::~AlarmStateCluster() = default;

void AlarmStateCluster::Refresh() {
  if (severity_tiles_)
    severity_tiles_->Refresh();

  const events::SeverityTileCounts counts = counts_();
  const events::AlarmEscalation escalation = events::EscalationFor(counts);

  // Annunciation: the chip states the condition rather than restating the
  // count, because the `Critical N` tile is right beside it — the same
  // division the web client settled on when its ladder moved into the bar.
  annunciator_->setVisible(escalation.annunciating);
  if (escalation.annunciating) {
    annunciator_->setText(QStringLiteral(" %1 ").arg(
        QString::fromStdU16String(Translate("Unacknowledged critical"))));
    // The first lit frame has to come from the clock too, or the chip shows
    // a stale phase until the first tick — which under a frozen clock is
    // forever, and is precisely the frame a capture takes.
    annunciator_flash_on_ =
        BlinkPhaseAt(scada::Now(), kAnnunciatorFlashHalfPeriod);
    StyleAnnunciator();
    if (!annunciator_flash_->isActive())
      annunciator_flash_->start();
  } else {
    annunciator_flash_->stop();
    // No phase to reset: it is a function of the clock, so the next alarm
    // picks it up wherever the clock is rather than inheriting whatever this
    // one left behind.
  }

  // Flood escalation: a single prominent state pill when the unacknowledged
  // count crosses the flood threshold.
  flood_pill_->setVisible(escalation.flooding);
  if (escalation.flooding) {
    flood_pill_->setText(
        QStringLiteral(" %1 (%2) ")
            .arg(QString::fromStdU16String(Translate("Alarm flood")))
            .arg(counts.unacknowledged));
    // A stylesheet, not the palette: the pill is a rounded fill, and
    // border-radius is one of the few things QPalette cannot express.
    //
    // The fill is a process-semantic colour, fixed by ISA-18.2 and exempt
    // from platform styling — but the text on it is derived from the fill
    // rather than baked. The dark and light critical tokens differ enough in
    // luminance that one constant cannot serve both, which the previous
    // hard-coded #ffffff did not account for.
    const QColor fill = CriticalFill();
    flood_pill_->setStyleSheet(
        QStringLiteral(
            "background:%1;color:%2;border-radius:%3px;font-weight:700;")
            .arg(fill.name(), scada::aui::ReadableTextOn(fill).name())
            .arg(PillRadius(*flood_pill_, 0)));
  }
}

// Both phases are severity-critical (a process-semantic colour, exempt from
// platform styling, §9) and differ in *fill*: the quiet phase is an outline —
// the mockup's `.ann` treatment, which distinguishes it from the flood pill's
// solid fill — and the lit phase fills. The caption keeps critical-token
// contrast in both, derived rather than baked, for the same reason the flood
// pill derives its text colour.
//
// A stylesheet rather than the palette, on the same grounds as the flood pill:
// the chip is a rounded fill with a border, and QPalette expresses neither.
void AlarmStateCluster::StyleAnnunciator() {
  const QColor accent = CriticalFill();
  const int radius = PillRadius(*annunciator_, 1);

  if (annunciator_flash_on_) {
    annunciator_->setStyleSheet(
        QStringLiteral("background:%1;color:%2;border:1px solid %1;"
                       "border-radius:%3px;font-weight:700;")
            .arg(accent.name(), scada::aui::ReadableTextOn(accent).name())
            .arg(radius));
  } else {
    annunciator_->setStyleSheet(
        QStringLiteral("background:transparent;color:%1;border:1px solid %1;"
                       "border-radius:%2px;font-weight:700;")
            .arg(accent.name())
            .arg(radius));
  }
}
