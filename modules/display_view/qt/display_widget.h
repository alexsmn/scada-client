#pragma once

#include "base/lifetime.h"
#include "display/view/display_document.h"

#include <QWidget>

#include <filesystem>
#include <functional>
#include <memory>

// Shows a VDS or Modus SDE/XSDE display, with live equipment state on it.
//
// This is the client's half of ADR 0012: the renderer is *linked*, not loaded,
// so the document is painted straight into this widget's own QPainter in
// paintEvent. The plugin it replaced blitted a BGRA buffer of exactly the
// widget's pixel size, which is why zooming and device-pixel-ratio correctness
// were unreachable before and are merely unimplemented now (task 748).
//
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

  // Called with the data source of the shape under a click, when there is one.
  using SelectionCallback = std::function<void(QString data_source)>;
  void set_selection_callback(SelectionCallback callback) {
    selection_callback_ = std::move(callback);
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
  QPointF WidgetToPage(const QPoint& point) const;

  std::unique_ptr<scada::display::view::DisplayDocument> document_;
  std::filesystem::path path_;
  QString title_;
  QString error_message_;

  SelectionCallback selection_callback_;
  DoubleClickCallback double_click_callback_;
};
