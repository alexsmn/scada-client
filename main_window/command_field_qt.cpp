#include "main_window/command_field_qt.h"

#include "aui/qt/image_util.h"

#include <QAction>
#include <QEvent>
#include <QFont>
#include <QFontMetrics>
#include <QIcon>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QStyle>
#include <QStyleOptionFrame>

#include <algorithm>
#include <cmath>
#include <utility>

namespace {

// The Lucide magnifier. `stroke="currentColor"`, so the tint is the consumer's
// (docs/client/ux/iconography.md §4) — here, the placeholder colour.
constexpr char kSearchGlyph[] = ":/icons/search.svg";

// How much smaller the shortcut hint is than the field's own text. Enough to
// read as an annotation, not so much that it stops being legible at the OS's
// smallest text size.
constexpr qreal kHintFontScale = 0.85;

}  // namespace

CommandField::CommandField(QWidget* parent,
                           QString prompt,
                           QKeySequence shortcut,
                           ActivateCallback on_activate)
    : QLineEdit{parent},
      shortcut_{std::move(shortcut)},
      on_activate_{std::move(on_activate)} {
  setObjectName(QStringLiteral("commandField"));
  setReadOnly(true);
  setPlaceholderText(prompt);
  // The prompt is the field's only label, so it is also its accessible name —
  // a screen reader on an empty read-only line edit would otherwise announce
  // nothing at all.
  setAccessibleName(std::move(prompt));

  // Preferred, not the QLineEdit default of Expanding: the context bar puts a
  // stretch on either side of the field, and an expanding field would eat both
  // and stop being centred.
  setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

  search_action_ = addAction(GlyphIcon(), QLineEdit::LeadingPosition);
  // The glyph is a button, so it swallows the clicks that would otherwise reach
  // the field. Give it the same meaning rather than a dead spot.
  connect(search_action_, &QAction::triggered, this,
          [this] { Activate(QString{}); });

  UpdateHintMargin();
}

QSize CommandField::sizeHint() const {
  const QSize base = QLineEdit::sizeHint();

  // QLineEdit sizes itself for a nominal number of characters and knows nothing
  // about the placeholder, the leading glyph or the hint. Ask for all three, so
  // the prompt states the field's purpose in full instead of eliding — and ask
  // in font and style metrics, so the answer moves with the OS text size.
  const QFontMetrics metrics{font()};
  const int icon_size =
      style()->pixelMetric(QStyle::PM_SmallIconSize, nullptr, this);
  const int width = metrics.horizontalAdvance(placeholderText()) + icon_size +
                    HintSize().width() + 4 * Gap() + base.width() / 4;

  return {std::max(base.width(), width), base.height()};
}

QString CommandField::HintText() const {
  // NativeText, so macOS gets `⌘K` and Windows `Ctrl+K` from the one
  // QKeySequence the shortcut itself was built from — no second spelling to
  // drift from the binding.
  return shortcut_.isEmpty() ? QString{}
                             : shortcut_.toString(QKeySequence::NativeText);
}

void CommandField::paintEvent(QPaintEvent* event) {
  QLineEdit::paintEvent(event);

  const QString text = HintText();
  const QRect rect = HintRect();
  if (text.isEmpty() || rect.isEmpty())
    return;

  // Same colour as the prompt: the hint is a caption on a field the operator is
  // not typing into, and must not compete with real content anywhere on screen.
  const QColor color = palette().color(QPalette::PlaceholderText);

  QPainter painter{this};
  painter.setRenderHint(QPainter::Antialiasing);
  painter.setPen(color);
  painter.setBrush(Qt::NoBrush);
  // Half-pixel inset so the hairline lands on the pixel grid rather than
  // straddling it.
  const qreal radius = rect.height() / 4.0;
  painter.drawRoundedRect(QRectF{rect}.adjusted(0.5, 0.5, -0.5, -0.5), radius,
                          radius);

  painter.setFont(HintFont());
  painter.drawText(rect, Qt::AlignCenter, text);
}

void CommandField::mouseReleaseEvent(QMouseEvent* event) {
  // Deliberately not forwarded: the field owns no text, so placing a cursor in
  // it would be a lie. A click means "open the palette".
  event->accept();
  Activate(QString{});
}

void CommandField::keyPressEvent(QKeyEvent* event) {
  const QString text = event->text();
  const bool has_command_modifier =
      event->modifiers() &
      (Qt::ControlModifier | Qt::AltModifier | Qt::MetaModifier);
  if (!has_command_modifier && !text.isEmpty() && text.at(0).isPrint()) {
    event->accept();
    Activate(text);
    return;
  }
  QLineEdit::keyPressEvent(event);
}

void CommandField::changeEvent(QEvent* event) {
  QLineEdit::changeEvent(event);

  switch (event->type()) {
    case QEvent::PaletteChange:
    case QEvent::ApplicationPaletteChange:
    case QEvent::StyleChange:
      ApplyGlyph();
      [[fallthrough]];
    case QEvent::FontChange:
      UpdateHintMargin();
      updateGeometry();
      update();
      break;
    default:
      break;
  }
}

void CommandField::Activate(const QString& initial_text) {
  if (on_activate_)
    on_activate_(initial_text);
}

QIcon CommandField::GlyphIcon() const {
  const int size =
      style()->pixelMetric(QStyle::PM_SmallIconSize, nullptr, this);
  return LoadTintedGlyph(kSearchGlyph, size,
                         palette().color(QPalette::PlaceholderText),
                         devicePixelRatioF());
}

void CommandField::ApplyGlyph() {
  if (search_action_)
    search_action_->setIcon(GlyphIcon());
}

QFont CommandField::HintFont() const {
  QFont hint_font = font();
  // A font carries either a point size or a pixel size; scale whichever it has.
  if (hint_font.pointSizeF() > 0) {
    hint_font.setPointSizeF(hint_font.pointSizeF() * kHintFontScale);
  } else if (hint_font.pixelSize() > 0) {
    hint_font.setPixelSize(std::max(
        1,
        static_cast<int>(std::lround(hint_font.pixelSize() * kHintFontScale))));
  }
  return hint_font;
}

QSize CommandField::HintSize() const {
  const QString text = HintText();
  // A zero size, not a default-constructed (-1, -1) one: callers do arithmetic
  // on the width, and an invalid QSize silently makes it negative.
  if (text.isEmpty())
    return {0, 0};

  const QFontMetrics metrics{HintFont()};
  const int horizontal_padding = std::max(2, metrics.averageCharWidth() / 2);
  const int vertical_padding = std::max(1, metrics.descent() / 2);
  return {metrics.horizontalAdvance(text) + 2 * horizontal_padding,
          metrics.height() + 2 * vertical_padding};
}

QRect CommandField::HintRect() const {
  const QSize size = HintSize();
  if (size.isEmpty())
    return {};

  // The style's own idea of where a line edit's contents live, so the chip sits
  // inside the frame the platform actually drew rather than one assumed here.
  QStyleOptionFrame option;
  initStyleOption(&option);
  const QRect contents =
      style()->subElementRect(QStyle::SE_LineEditContents, &option, this);

  QRect rect{QPoint{0, 0}, size};
  rect.moveCenter(contents.center());
  rect.moveRight(contents.right() - Gap());
  return rect;
}

int CommandField::Gap() const {
  return std::max(2, QFontMetrics{font()}.averageCharWidth() / 2);
}

void CommandField::UpdateHintMargin() {
  // Right margin = the chip plus a gap on each side of it, so the prompt stops
  // before the chip instead of running under it.
  const QSize size = HintSize();
  setTextMargins(0, 0, size.isEmpty() ? 0 : size.width() + 2 * Gap(), 0);
}
