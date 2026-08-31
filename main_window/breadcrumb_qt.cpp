#include "main_window/breadcrumb_qt.h"

#include <QEvent>
#include <QFont>
#include <QFontMetrics>
#include <QHBoxLayout>
#include <QLabel>
#include <QMargins>
#include <QResizeEvent>
#include <QSize>
#include <QtGlobal>

#include <vector>

namespace {

// The separator between steps. A forward slash is what the mockup screens draw
// and what every file path and web breadcrumb uses, so it needs no legend.
constexpr QLatin1String kSeparator{" / "};

// Below this the path is not worth drawing at all — a couple of ellipses and a
// slash state nothing. The breadcrumb hides instead, and the command field
// takes the room.
constexpr int kMinimumUsefulWidth = 60;

}  // namespace

Breadcrumb::Breadcrumb(QWidget* parent) : QWidget(parent) {
  layout_ = new QHBoxLayout(this);
  layout_->setContentsMargins(0, 0, 0, 0);
  // The separator label carries its own padding (" / "), so the layout adds
  // none — spacing here would double it, and unlike the separator's spaces it
  // would not scale with the font.
  layout_->setSpacing(0);
  // The layout must not push a minimum size onto this widget. Its default
  // constraint does exactly that for a non-window widget, and since the steps
  // are laid out at explicit widths that minimum is "whatever the path
  // currently occupies" — which the bar could then never shrink, so no resize
  // would arrive and no re-elision would happen. The widget states its own
  // floor in minimumSizeHint(), and it is zero.
  layout_->setSizeConstraint(QLayout::SetNoConstraint);
  // The breadcrumb yields space to the rest of the bar: it elides, where the
  // command field and the severity tiles cannot usefully shrink. The explicit
  // zero minimum is what makes that true — without it the layout's own minimum
  // (the sum of the labels') becomes a floor the widget cannot go below.
  //
  // `Maximum` rather than `Ignored`: the widget wants at most its untruncated
  // path (sizeHint) and will give up any of it. `Ignored` said the opposite —
  // that the hint means nothing — and a toolbar duly gave the breadcrumb its
  // minimum, which is this same zero. Every step then elided away and the bar
  // drew the separators alone.
  setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);
  setMinimumWidth(0);
}

void Breadcrumb::SetSegments(std::span<const Segment> segments) {
  std::vector<Segment> next;
  next.reserve(segments.size());
  for (const Segment& segment : segments) {
    if (segment.label.isEmpty())
      continue;
    // A step that repeats the one before it states nothing twice. The path is
    // page / view / subject, and for a view named after what it is pointed at
    // the last two coincide: a Graph tab on one series takes its title from
    // that series (`GraphView::MakeTitle`), which is the same display name the
    // subject reads. The emphasis is merged rather than dropped, so the
    // surviving step keeps the subject's weight.
    if (!next.empty() && next.back().label == segment.label) {
      next.back().strong = next.back().strong || segment.strong;
      continue;
    }
    next.push_back(segment);
  }
  if (next == segments_)
    return;
  segments_ = std::move(next);
  Rebuild();
}

QString Breadcrumb::Text() const {
  QString text;
  for (const Segment& segment : segments_) {
    if (!text.isEmpty())
      text += kSeparator;
    text += segment.label;
  }
  return text;
}

void Breadcrumb::Rebuild() {
  // Tear down the previous row wholesale. Cheap on a path this short.
  labels_.clear();
  separators_.clear();
  while (QLayoutItem* item = layout_->takeAt(0)) {
    delete item->widget();
    delete item;
  }

  for (std::size_t i = 0; i < segments_.size(); ++i) {
    if (i > 0) {
      auto* separator = new QLabel(kSeparator, this);
      // Quiet, like the middle steps: the separator is punctuation, not
      // content.
      separator->setForegroundRole(QPalette::PlaceholderText);
      separators_.push_back(separator);
      layout_->addWidget(separator);
    }

    auto* label = new QLabel(this);
    // Each step is given an explicit width by ApplyElision, so the layout
    // allocates nothing here.
    //
    // Letting it allocate is what the two previous attempts did, and neither
    // policy can work: a QLabel's minimumSizeHint is its full string, so
    // anything that respects hints makes the longest path a floor the
    // breadcrumb cannot shrink below; and `Ignored`, which zeroes that floor,
    // also zeroes the hint, leaving the layout to split the row *equally* with
    // no idea what each step needs — a long step clipped to a third of the
    // widget while a short one sat in slack it could not use. Sizing each
    // label to the text it ended up with settles both, because the text has
    // already been fitted to the width the widget was granted.
    if (segments_[i].strong) {
      QFont bold = font();
      bold.setBold(true);
      label->setFont(bold);
    } else {
      label->setForegroundRole(QPalette::PlaceholderText);
    }
    // The untruncated step is the tooltip, so elision never costs the operator
    // the name.
    label->setToolTip(segments_[i].label);
    labels_.push_back(label);
    layout_->addWidget(label);
  }
  // The steps are fixed-width, so the stretch is the only elastic item and it
  // simply parks the slack on the right — the path left-aligns in its slot.
  layout_->addStretch();

  // Settle the parent's layout before eliding, rather than eliding against the
  // width the previous path was given.
  //
  // updateGeometry() only *posts* a layout request, so without this the first
  // ApplyElision runs one pass behind: a path that has just grown is fitted
  // into the old, narrower slot and comes out truncated, and only the next
  // resize puts it right. A running client hides that behind the following
  // event cycle; the headless screenshot generator grabs the frame in between,
  // which is how it kept rendering a fully-elided breadcrumb inside a slot
  // with room to spare.
  updateGeometry();
  if (QWidget* parent = parentWidget()) {
    if (QLayout* parent_layout = parent->layout())
      parent_layout->activate();
  }
  ApplyElision();
  setVisible(!segments_.empty());
}

QFont Breadcrumb::StepFont(const Segment& segment) const {
  QFont step = font();
  step.setBold(segment.strong);
  return step;
}

int Breadcrumb::SeparatorWidth() const {
  if (segments_.size() < 2)
    return 0;
  return QFontMetrics{font()}.horizontalAdvance(kSeparator) *
         static_cast<int>(segments_.size() - 1);
}

std::vector<int> Breadcrumb::NaturalWidths() const {
  std::vector<int> widths(segments_.size());
  for (std::size_t i = 0; i < segments_.size(); ++i) {
    widths[i] = QFontMetrics{StepFont(segments_[i])}.horizontalAdvance(
        segments_[i].label);
  }
  return widths;
}

QSize Breadcrumb::sizeHint() const {
  int width = SeparatorWidth();
  for (int natural : NaturalWidths())
    width += natural;

  const QMargins margins = contentsMargins();
  return QSize{
      width + margins.left() + margins.right(),
      QFontMetrics{font()}.height() + margins.top() + margins.bottom()};
}

QSize Breadcrumb::minimumSizeHint() const {
  const QMargins margins = contentsMargins();
  return QSize{
      0, QFontMetrics{font()}.height() + margins.top() + margins.bottom()};
}

void Breadcrumb::ApplyElision() {
  if (labels_.empty())
    return;

  // Width the steps may share, once the separators have taken theirs.
  const int available = width() - SeparatorWidth();
  if (available < kMinimumUsefulWidth) {
    // Blanking the steps is not enough on its own: the separators are content
    // of their own, and a bar drawing `/ /` with nothing between the slashes
    // claims there is a path and then declines to name it. Hide the punctuation
    // with the steps so the slot goes honestly empty — which is what the
    // comment on kMinimumUsefulWidth has always said this branch does.
    for (QLabel* label : labels_) {
      label->setText(QString{});
      label->setFixedWidth(0);
    }
    for (QLabel* separator : separators_)
      separator->setVisible(false);
    return;
  }
  for (QLabel* separator : separators_)
    separator->setVisible(true);

  // What each step would need in full — from the same measurement sizeHint()
  // uses, so the two cannot disagree about whether the path fits. They did
  // when this read `labels_[i]->font()` instead: a label resolves its font
  // from its parent chain once shown, which is not always the font sizeHint
  // measured with, and a few pixels of drift is the difference between "the
  // path fits" and eliding inside the width the widget just asked for.
  const std::vector<int> natural = NaturalWidths();
  int wanted = 0;
  for (int step : natural)
    wanted += step;

  // The whole path fits: draw it. This case has to be checked rather than
  // fallen into, because the share-out below is a *response to shortage* and
  // an even one truncates inside a width that was never short. That is not
  // hypothetical — sizeHint() asks the layout for exactly `wanted`, so the
  // common case is landing on precisely this boundary, and the even split
  // spent a short step's slack on nothing while eliding the long step beside
  // it: `Page 1 / Objects / Feeder bay 12` rendered as `Page 1 / Objects /
  // Fee… 12` in the width it had just asked for.
  if (wanted <= available) {
    for (std::size_t i = 0; i < labels_.size(); ++i) {
      labels_[i]->setText(segments_[i].label);
      labels_[i]->setFixedWidth(natural[i]);
    }
    return;
  }

  // Genuinely short, so every step gives up something — in proportion to what
  // it asked for, which is the property the even split was reaching for. A
  // long device name beside two short steps keeps the slack it needs, and no
  // step is starved to nothing, because a share of a positive width is
  // positive.
  for (std::size_t i = 0; i < labels_.size(); ++i) {
    const int budget = wanted > 0
                           ? static_cast<int>(static_cast<qint64>(available) *
                                              natural[i] / wanted)
                           : 0;
    const QFontMetrics metrics{StepFont(segments_[i])};
    const QString elided =
        metrics.elidedText(segments_[i].label, Qt::ElideMiddle, budget);
    labels_[i]->setText(elided);
    labels_[i]->setFixedWidth(metrics.horizontalAdvance(elided));
  }
}

void Breadcrumb::resizeEvent(QResizeEvent* event) {
  QWidget::resizeEvent(event);
  ApplyElision();
}

void Breadcrumb::changeEvent(QEvent* event) {
  QWidget::changeEvent(event);
  if (event->type() == QEvent::FontChange ||
      event->type() == QEvent::PaletteChange ||
      event->type() == QEvent::ApplicationPaletteChange) {
    // The bold weight is derived from the widget font, so a font change has to
    // rebuild rather than just re-elide.
    Rebuild();
  }
}
