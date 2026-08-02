#include "main_window/breadcrumb_qt.h"

#include <QEvent>
#include <QFontMetrics>
#include <QHBoxLayout>
#include <QLabel>
#include <QResizeEvent>

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
  // The breadcrumb yields space to the rest of the bar: it elides, where the
  // command field and the severity tiles cannot usefully shrink. The explicit
  // zero minimum is what makes that true — without it the layout's own minimum
  // (the sum of the labels') becomes a floor the widget cannot go below.
  setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
  setMinimumWidth(0);
}

void Breadcrumb::SetSegments(std::span<const Segment> segments) {
  std::vector<Segment> next;
  next.reserve(segments.size());
  for (const Segment& segment : segments) {
    if (!segment.label.isEmpty())
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
  // Tear down the previous row wholesale. Reusing labels would mean tracking
  // which of them are separators, for no measurable gain on a path this short.
  labels_.clear();
  while (QLayoutItem* item = layout_->takeAt(0)) {
    delete item->widget();
    delete item;
  }

  for (std::size_t i = 0; i < segments_.size(); ++i) {
    if (i > 0) {
      auto* separator = new QLabel(kSeparator, this);
      // Quiet, like the middle steps: the separator is punctuation, not content.
      separator->setForegroundRole(QPalette::PlaceholderText);
      layout_->addWidget(separator);
    }

    auto* label = new QLabel(this);
    // The label must not make its text width a layout minimum. A QLabel's
    // default size hint is the full string, and QHBoxLayout treats that as a
    // floor — so the breadcrumb could not shrink below its longest path, the
    // elision below never fired, and in the toolbar it shoved the command field
    // aside instead of getting out of the way. Elision is computed here, so the
    // hint is not wanted.
    label->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
    label->setMinimumWidth(0);
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
  layout_->addStretch();

  ApplyElision();
  setVisible(!segments_.empty());
}

void Breadcrumb::ApplyElision() {
  if (labels_.empty())
    return;

  // Width the steps may share, once the separators have taken theirs. Measured
  // per label rather than assumed, because a bold step is wider than a quiet
  // one at the same character count.
  const int separator_width =
      QFontMetrics{font()}.horizontalAdvance(kSeparator) *
      static_cast<int>(labels_.size() - 1);
  const int available = width() - separator_width;
  if (available < kMinimumUsefulWidth) {
    for (QLabel* label : labels_)
      label->setText(QString{});
    return;
  }

  // Share what is left evenly. An even split is not the cleverest policy — a
  // long device name beside two short steps would rather have the slack — but
  // it is the one that cannot starve a step to nothing, which is what matters
  // when every step is a name the operator is trying to read.
  const int per_label = available / static_cast<int>(labels_.size());
  for (std::size_t i = 0; i < labels_.size(); ++i) {
    const QFontMetrics metrics{labels_[i]->font()};
    labels_[i]->setText(
        metrics.elidedText(segments_[i].label, Qt::ElideMiddle, per_label));
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
