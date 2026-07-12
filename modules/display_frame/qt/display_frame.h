#pragma once

#include <QSize>
#include <QString>
#include <QWidget>

class VdsRuntimeWidget;
class QLabel;
class QScrollArea;

// Pure zoom helpers (no widget state) so the frame's scaling maths can be
// unit-tested without a running QApplication.

// Clamps a zoom factor to the frame's supported range. Values outside
// [kMinZoom, kMaxZoom] would either collapse the page to nothing or blow the
// scroll area up past what QWidget sizes allow.
double ClampDisplayZoom(double zoom);

// The zoom factor that fits a page of `natural` size inside `viewport` while
// preserving aspect ratio (contain). Returns 1.0 for degenerate inputs so an
// unmeasured viewport shows the page at 100% rather than vanishing.
double DisplayFitFactor(QSize natural, QSize viewport);

// The zoom factor rendered as the toolbar's integer percent label (e.g. 1.0 ->
// 100). Rounds to the nearest percent.
int DisplayZoomPercent(double zoom);

// A reshelled chrome frame around a schematic (single-line) display renderer.
//
// It wraps the cross-platform VDS renderer widget with the workbench display
// toolbar — a Live indicator, a hotspot breadcrumb, and a zoom / fit / 100% /
// export control — matching
// client/docs/ui-mockups/screens/substation-display.html. The diagram geometry
// is unchanged: the frame only hosts the renderer in a scrollable viewport
// whose zoom scales the rendered page. Selection, hit-testing and data binding
// stay on the wrapped renderer.
//
// Opt-in: construct this only under the reshell UX theme (see
// WrapDisplayInFrame); the legacy look keeps the bare renderer.
class DisplayFrame : public QWidget {
  Q_OBJECT

 public:
  // `diagram` is reparented into the frame's scroll viewport (the frame takes
  // ownership). `breadcrumb` is the human-readable display location shown at
  // the toolbar's left (typically the display title); it may be empty.
  DisplayFrame(VdsRuntimeWidget* diagram,
               QString breadcrumb,
               QWidget* parent = nullptr);
  ~DisplayFrame() override;

 protected:
  // Refits on the scroll viewport's own resize (installed as an event filter),
  // not the frame's — the viewport is sized only after the frame's layout runs,
  // so keying off the frame resize would fit against a stale viewport size.
  bool eventFilter(QObject* watched, QEvent* event) override;

 private:
  void BuildToolbar(const QString& breadcrumb);
  QSize DiagramNaturalSize() const;
  void ApplyZoom();
  void RefitToViewport();
  void SetZoom(double zoom);
  void ExportImage();

  VdsRuntimeWidget* diagram_ = nullptr;
  QScrollArea* scroll_ = nullptr;
  QLabel* zoom_label_ = nullptr;

  double zoom_ = 1.0;
  // While true, the page is kept fitted to the viewport on every resize. The
  // first explicit zoom / 100% action pins the zoom and clears this.
  bool fit_ = true;
};

// Wraps `diagram` in a DisplayFrame when the reshell UX theme is active
// (scada::aui::GetSeverityTheme() != SeverityTheme::kLegacy). Otherwise returns
// `diagram` unchanged so the legacy build keeps the bare renderer. Ownership of
// the returned widget transfers to the caller; the returned frame owns
// `diagram`.
QWidget* WrapDisplayInFrame(VdsRuntimeWidget* diagram, QString breadcrumb);
