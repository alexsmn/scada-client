#pragma once

#include <QString>
#include <QWidget>

// The hint a graph draws while it plots nothing: a title saying so and a line
// saying how to fill it (backlog 1125). Without it the Overview's dominant
// trend was two thirds of the workspace showing only a time axis, with no clue
// that it is meant to be filled or how (docs/client/ux/principles.md §4).
//
// It is a child laid over the whole graph and transparent to the mouse, so the
// graph's own context menu, panes and axes keep working underneath. Its owner
// shows it while no pane holds a line and raises it whenever it does, because
// panes created later stack above it. It paints with palette roles and font
// metrics only, so it follows the theme and the platform font.
//
// The hint names the object tree's checkbox because that is how a signal
// reaches an open graph (ObjectTreeView's checked handler adds it to the
// active view's ContentsModel). The graph accepts no drops, so it does not
// suggest dragging.
class GraphEmptyState : public QWidget {
 public:
  // Lays the hint over |graph| and keeps it sized to it.
  explicit GraphEmptyState(QWidget& graph);

  // The first line: what is wrong.
  static QString TitleText();
  // The second line: what to do about it.
  static QString HintText();

 protected:
  // QObject — follows the graph's resizes.
  bool eventFilter(QObject* watched, QEvent* event) override;

  // QWidget
  void paintEvent(QPaintEvent* event) override;
};
