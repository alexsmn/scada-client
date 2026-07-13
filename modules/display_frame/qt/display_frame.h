#pragma once

#include "events/event_observer.h"

#include <QSize>
#include <QString>
#include <QWidget>

#include <memory>
#include <vector>

namespace scada {
class NodeId;
}
class NodeEventProvider;
class NodeService;
class QLabel;
class QScrollArea;
class QTableWidget;
class TimedDataService;
class TimedDataSpec;
class VdsRuntimeWidget;

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

// The Recent-events strip's severity label for an OPC UA event severity
// (0-1000-style `scada::EventSeverity`): "Critical" / "Warning" / "Info". Pure
// and translation-free at the mapping level (callers translate the result).
enum class DisplaySeverityBand { kInfo, kWarning, kCritical };
DisplaySeverityBand DisplaySeverityBandFor(unsigned severity);

// Live-data sources for the display frame's bay strips. All optional: when a
// pointer is null the corresponding strip is omitted, so the frame degrades to
// just the toolbar + diagram.
struct DisplayFrameContext {
  // Backs the Measurements strip: each selected element becomes a live row.
  TimedDataService* timed_data_service = nullptr;
  // Backs the Recent-events strip (its unacknowledged/actionable events).
  NodeEventProvider* node_event_provider = nullptr;
  // Resolves an event's source node to a display name for the Object column.
  NodeService* node_service = nullptr;
};

// A reshelled chrome frame around a schematic (single-line) display renderer.
//
// It wraps the cross-platform VDS renderer widget with the workbench display
// toolbar — a Live indicator, a hotspot breadcrumb, and a zoom / fit / 100% /
// export control — and, when live-data sources are supplied, the bay strips
// below the diagram: a Measurements watch (the elements the operator selects)
// and a Recent-events list (reusing the event surface). Matches
// client/docs/ui-mockups/screens/substation-display.html. The diagram geometry
// is unchanged: the frame only hosts the renderer in a scrollable viewport
// whose zoom scales the rendered page.
//
// Opt-in: construct this only under the reshell UX theme (see
// WrapDisplayInFrame); the legacy look keeps the bare renderer.
class DisplayFrame : public QWidget, private EventObserver {
  Q_OBJECT

 public:
  // `diagram` is reparented into the frame's scroll viewport (the frame takes
  // ownership). `breadcrumb` is the human-readable display location shown at
  // the toolbar's left (typically the display title); it may be empty.
  // `data_context` supplies the bay strips' live sources (all optional).
  DisplayFrame(VdsRuntimeWidget* diagram,
               QString breadcrumb,
               DisplayFrameContext data_context,
               QWidget* parent = nullptr);
  ~DisplayFrame() override;

  // Adds (or moves to the front) a signal in the Measurements strip and starts
  // showing its live value. No-op without a timed-data service. Called as the
  // operator selects diagram elements.
  void ShowMeasurement(const scada::NodeId& node_id);

 protected:
  // Refits on the scroll viewport's own resize (installed as an event filter),
  // not the frame's — the viewport is sized only after the frame's layout runs,
  // so keying off the frame resize would fit against a stale viewport size.
  bool eventFilter(QObject* watched, QEvent* event) override;

 private:
  void BuildToolbar(const QString& breadcrumb);
  QWidget* BuildBayStrips();
  QSize DiagramNaturalSize() const;
  void ApplyZoom();
  void RefitToViewport();
  void SetZoom(double zoom);
  void ExportImage();

  void RefreshMeasurementRow(int row, const TimedDataSpec& spec);
  void RefreshEvents();

  // EventObserver (Recent-events strip refresh triggers).
  void OnEvents(std::span<const scada::Event* const> events) override;
  void OnAllEventsAcknowledged() override;

  DisplayFrameContext data_context_;

  VdsRuntimeWidget* diagram_ = nullptr;
  QScrollArea* scroll_ = nullptr;
  QLabel* zoom_label_ = nullptr;

  QTableWidget* measurements_ = nullptr;
  QTableWidget* events_ = nullptr;

  // One spec per Measurements row (parallel to the table's rows); each spec's
  // update_handler refreshes its row live.
  std::vector<std::unique_ptr<TimedDataSpec>> measurement_specs_;

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
QWidget* WrapDisplayInFrame(VdsRuntimeWidget* diagram,
                            QString breadcrumb,
                            DisplayFrameContext data_context);
