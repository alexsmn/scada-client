#pragma once

#include <QWidget>

#include <span>
#include <vector>

class QHBoxLayout;
class QLabel;

// The top context bar's breadcrumb — where the workspace currently is.
//
// docs/client/ux/shell.md §2.2 settles the bar's contract: one persistent bar,
// whose left/centre slot carries the command field *plus* a breadcrumb when the
// active surface has a path worth stating. The two coexist and answer different
// questions — the field is what you can do next, the breadcrumb is where you
// already are. No mockup screen draws both only because each screen shows one
// situation.
//
// It states page → view → selected object, which is a fact no other region
// carries: the activity rail says which pane mode and which page, the workspace
// tab says which view, but nothing names the object the view is pointed at.
//
// Native by construction, and deliberately so (shell.md §9):
//
//  - **No stylesheet, no token hex.** Emphasis is a bold weight derived from
//    the widget's own font, and the de-emphasised segments take
//    `QPalette::PlaceholderText` through `setForegroundRole`, so a theme or OS
//    appearance change repaints them with no code.
//  - **No fixed widths.** Elision is computed from `QFontMetrics` against the
//    width the layout actually grants, so the breadcrumb tracks the OS
//    text-size setting instead of assuming a pixel budget. This is the same
//    rule `CommandField` established when it dropped `setFixedWidth(360)`.
class Breadcrumb : public QWidget {
  Q_OBJECT

 public:
  // One step of the path. `strong` renders in the foreground weight — the ends
  // of the path (the page and the subject), per the mockup screens; the middle
  // is context and stays quiet.
  struct Segment {
    QString label;
    bool strong = false;

    // So SetSegments can skip a rebuild when nothing moved. Navigation
    // refreshes fire on every selection change, most of which do not alter the
    // path.
    bool operator==(const Segment&) const = default;
  };

  explicit Breadcrumb(QWidget* parent);

  // Replaces the whole path. Empty labels are dropped, so a caller can pass a
  // fixed-arity path and let the absent steps fall away — a view with no
  // selection simply has no third segment rather than a dangling separator.
  void SetSegments(std::span<const Segment> segments);

  // The path as rendered, separators included, for tests and accessibility.
  QString Text() const;

 protected:
  // QWidget — re-elides against the granted width. Elision cannot be decided at
  // SetSegments time because the layout has not yet said how much room there is.
  void resizeEvent(QResizeEvent* event) override;
  // QWidget — a font or palette change invalidates the elision and the
  // de-emphasis, both of which are derived rather than stored.
  void changeEvent(QEvent* event) override;

 private:
  // Rebuilds the label row from `segments_`. Cheap enough to redo wholesale:
  // the path is at most a handful of steps and only changes on navigation.
  void Rebuild();
  // Applies elision to the current labels for the given available width.
  void ApplyElision();

  std::vector<Segment> segments_;
  QHBoxLayout* layout_ = nullptr;
  // The value labels, in path order, parallel to `segments_`. Separators are
  // owned by the layout and need no tracking.
  std::vector<QLabel*> labels_;
};
