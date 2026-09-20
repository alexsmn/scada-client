#pragma once

#include "base/lifetime.h"
#include "display/view/display_document.h"

#include <QRectF>
#include <QSize>
#include <QString>
#include <QWidget>

#include <filesystem>
#include <functional>
#include <memory>
#include <optional>

// Shows a VDS or Modus SDE/XSDE display, with live equipment state on it.
//
// This is the client's half of ADR 0012: the renderer is *linked*, not loaded,
// so the document is painted straight into this widget's own QPainter in
// paintEvent. The plugin it replaced blitted a BGRA buffer of exactly the
// widget's pixel size, which is why zooming and device-pixel-ratio correctness
// were unreachable before; both come for free from painting into the widget's
// own painter, and ADR 0012 phase 3 delivered them by removing the buffer.
//
// Pure geometry and naming helpers (no widget state) so the selection maths
// can be unit-tested without a running QApplication.

// Maps a page-coordinate rectangle into the widget's own coordinates.
//
// Page Y grows upward and widget Y downward, so the rectangle is flipped about
// the page height; this is the exact inverse of the widget->page mapping the
// hit test uses, and the two must stay that way or the halo lands somewhere
// the click did not. Returns a null rect for a degenerate page or widget,
// which callers treat as "nothing to draw".
QRectF DisplayPageRectToWidget(const scada::display::RectF& page_rect,
                               const scada::display::RectF& page_bounds,
                               QSize widget_size);

// What to call a selected shape in operator-facing chrome.
//
// The authored name first -- it is the tag the operator knows the device by,
// and it is also the data-source binding -- then the drawn text, which is what
// a shape with no name shows on the diagram. Empty when the shape offers
// neither, in which case the chrome says nothing rather than inventing an id.
QString DisplayShapeLabel(const scada::display::view::ShapeHit& hit);

// A document that will not open is not an error the operator can act on
// through a dialog, so the failure is painted where the display would have
// been — the same treatment the plugin-era widget gave a missing library.
class DisplayWidget : public QWidget {
  Q_OBJECT

 public:
  explicit DisplayWidget(QWidget* parent = nullptr);
  ~DisplayWidget() override;

  // Opens `path`, choosing the parser with `kind`. Returns false and paints
  // the reason on failure.
  bool Open(const std::filesystem::path& path,
            scada::display::view::DocumentKind kind =
                scada::display::view::DocumentKind::kAuto);

  const QString& error_message() const SCADA_LIFETIME_BOUND {
    return error_message_;
  }
  const std::filesystem::path& path() const SCADA_LIFETIME_BOUND {
    return path_;
  }
  QString title() const { return title_; }

  // Called when the operator's selection changes: with the shape under the
  // click, or nullopt when the click landed on bare page and cleared it.
  //
  // It carried only the data source until ADR 0012 phase 5. The whole hit
  // travels now because the host needs more than the binding -- the status
  // strip names the shape -- and because a *cleared* selection has no data
  // source to report at all, which is why the old signature could not express
  // one.
  using SelectionCallback = std::function<void(
      const std::optional<scada::display::view::ShapeHit>& hit)>;
  void set_selection_callback(SelectionCallback callback) {
    selection_callback_ = std::move(callback);
  }

  // The shape the operator last clicked, or nullopt when the selection is
  // clear. Painted as the accent halo over the authored drawing.
  const std::optional<scada::display::view::ShapeHit>& selection() const
      SCADA_LIFETIME_BOUND {
    return selection_;
  }

  using DoubleClickCallback = std::function<void()>;
  void set_double_click_callback(DoubleClickCallback callback) {
    double_click_callback_ = std::move(callback);
  }

  QSize sizeHint() const override;

 protected:
  void paintEvent(QPaintEvent* event) override;
  void mousePressEvent(QMouseEvent* event) override;
  void mouseDoubleClickEvent(QMouseEvent* event) override;

 private:
  void PaintMessage(QPainter& painter, const QString& message) const;
  void PaintSelection(QPainter& painter) const;
  QPointF WidgetToPage(const QPoint& point) const;
  void SetSelection(std::optional<scada::display::view::ShapeHit> selection);

  std::unique_ptr<scada::display::view::DisplayDocument> document_;
  std::filesystem::path path_;
  QString title_;
  QString error_message_;

  std::optional<scada::display::view::ShapeHit> selection_;

  SelectionCallback selection_callback_;
  DoubleClickCallback double_click_callback_;
};
