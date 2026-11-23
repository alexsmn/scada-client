#include "graph/graph_empty_state.h"

#include "aui/translation.h"

#include <algorithm>

#include <QEvent>
#include <QFontMetrics>
#include <QPainter>

GraphEmptyState::GraphEmptyState(QWidget& graph) : QWidget{&graph} {
  setObjectName(QStringLiteral("graphEmptyState"));
  setAttribute(Qt::WA_TransparentForMouseEvents);
  setAttribute(Qt::WA_NoSystemBackground);
  setGeometry(graph.rect());
  graph.installEventFilter(this);
}

// static
QString GraphEmptyState::TitleText() {
  return QString::fromStdU16String(Translate("No signals on the graph"));
}

// static
QString GraphEmptyState::HintText() {
  return QString::fromStdU16String(
      Translate("Tick a signal in the Objects tree to plot it"));
}

bool GraphEmptyState::eventFilter(QObject* watched, QEvent* event) {
  if (watched == parentWidget() && event->type() == QEvent::Resize)
    setGeometry(parentWidget()->rect());
  return QWidget::eventFilter(watched, event);
}

void GraphEmptyState::paintEvent(QPaintEvent* /*event*/) {
  QPainter painter{this};

  QFont title_font = font();
  title_font.setBold(true);
  const QFontMetrics title_metrics{title_font};
  const QFontMetrics hint_metrics{font()};

  // Kept clear of the edges by one line height, so a narrow graph elides the
  // text rather than running it into the axes.
  const int margin = hint_metrics.height();
  const int width = std::max(0, this->width() - 2 * margin);
  const int gap = hint_metrics.height() / 2;
  const int block_height = title_metrics.height() + gap + hint_metrics.height();
  const int top = (height() - block_height) / 2;

  painter.setFont(title_font);
  painter.setPen(palette().color(QPalette::WindowText));
  painter.drawText(
      QRect{margin, top, width, title_metrics.height()},
      Qt::AlignHCenter | Qt::AlignTop,
      title_metrics.elidedText(TitleText(), Qt::ElideRight, width));

  painter.setFont(font());
  painter.setPen(palette().color(QPalette::PlaceholderText));
  painter.drawText(QRect{margin, top + title_metrics.height() + gap, width,
                         hint_metrics.height()},
                   Qt::AlignHCenter | Qt::AlignTop,
                   hint_metrics.elidedText(HintText(), Qt::ElideRight, width));
}
