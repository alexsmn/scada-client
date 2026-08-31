#pragma once

#include <QColor>
#include <QString>
#include <QWidget>

#include <functional>
#include <memory>
#include <optional>
#include <vector>

namespace scada {
class DataValue;
class NodeId;
class Qualifier;
}  // namespace scada
class NodeRef;
class SelectionModel;
class TimedDataSpec;
class QLabel;
class QPushButton;
class QStackedWidget;

// The quality band for the Inspector's state-hero pill. A pure mapping so it
// can be unit-tested without a running QApplication.
//
// kUnknown is the "nothing has ever been delivered" band. It exists because a
// default-constructed Qualifier is zero, and zero is *not* BAD — so a value
// that never arrived is indistinguishable from a good measurement at the
// Qualifier level. Presenting absent data as good is exactly the failure mode
// the HMI principles forbid (see docs/client/ux/principles.md).
enum class InspectorQualityBand { kGood, kBad, kUnknown };
InspectorQualityBand InspectorQualityBandFor(const scada::Qualifier& qualifier);
// Prefer this overload wherever a whole DataValue is at hand: it can tell an
// undelivered value from a good one, which the Qualifier alone cannot.
InspectorQualityBand InspectorQualityBandFor(const scada::DataValue& value);

// One configured limit band in the Measurements section.
struct InspectorLimitRow {
  // The band's short name, already translated ("HiHi", "Lo", …).
  QString label;
  // The limit, formatted with the node's own value format.
  QString value;
  // Whether the current value sits in this band — the row the operator needs
  // to see when a value is coloured.
  bool breached = false;
};

// One step of the selected event's lifecycle in the alarm card's History
// section.
struct InspectorTimelineRow {
  // The step's time, or empty for a step that has not happened yet.
  QString time;
  // The step's translated label.
  QString text;
};

// The alarm card's contents, for a journal-event selection.
struct InspectorEventView {
  QString source;
  QString node_id_text;
  QString message;
  unsigned severity = 0;
  QString time_text;
  QString acknowledged_text;
  bool acknowledgeable = false;
  bool source_available = false;
  // The event's lifecycle, oldest step first. Empty hides the section.
  std::vector<InspectorTimelineRow> timeline;
};

// The element card's contents. Grouped into a struct rather than a positional
// parameter list so the widget tests and the capture can fill exactly the
// parts they exercise.
struct InspectorElementView {
  QString title;
  QString node_id_text;
  QString value_text;
  // Defaults to kUnknown so a card filled without a live spec cannot claim
  // good quality for a readout it never received.
  InspectorQualityBand quality = InspectorQualityBand::kUnknown;
  QString updated_text;
  // The node's configured limit bands, most severe first. Empty when the node
  // configures none, which hides the limits block entirely.
  std::vector<InspectorLimitRow> limits;
  bool controllable = false;
  // Why control is unavailable, shown under the disabled Control button.
  // Empty when control is available, or when the reason is not known.
  QString control_reason;
};

// The plotted-series section's contents, for a selection made in a chart view.
// Everything the old in-tab series panel also showed — identity, limit bands,
// node and quality — the element card above it already renders from the
// selection, so this carries only what is not derivable from the node: how the
// series is drawn.
struct InspectorSeriesView {
  // The colour the series is plotted in; the matching palette swatch is ringed.
  QColor color;
  // Read-outs, not controls: the operator toggles these through the chart's own
  // commands, and the section reports what they currently are.
  bool own_pane = false;
  bool dots = false;
  bool stepped = false;
};

// Action wiring for the Inspector panel.
struct InspectorPanelContext {
  // Triggers the selection-scoped control/write command — the existing
  // two-stage confirm flow. Wired by the host to ExecuteCommand(ID_WRITE).
  std::function<void()> on_control;
  // Whether the control command is currently enabled for the active selection.
  std::function<bool()> is_control_enabled;
  // Why control is unavailable, asked only while it is. The host answers
  // because it owns the command resolution and the session; the panel just
  // renders the sentence.
  std::function<QString()> control_reason;
  // Acknowledges the selected journal event — the journal's own command
  // (the host wires ExecuteCommand(ID_ACKNOWLEDGE_CURRENT) through the
  // shell's command resolution).
  std::function<void()> on_acknowledge;
  // Whether acknowledging is currently possible for the active selection.
  std::function<bool()> is_acknowledge_enabled;
  // Opens the selected event's source in a graph — the selection-scoped
  // ID_OPEN_GRAPH command over the event's source node.
  std::function<void()> on_go_to_source;
  // Whether the source can be opened (the source node resolved and the graph
  // command accepts the selection).
  std::function<bool()> is_go_to_source_enabled;
  // Makes the selected node's limit bands readable, then calls `redraw`.
  //
  // The panel asks rather than fetching: the bands live on the node's property
  // children, which a selection never makes resident (see FetchLimitBands in
  // modules/inspector/limit_band.h), and the fetch needs the host's executor.
  // Unwired, the Measurements block simply stays hidden — which is what every
  // operator selection produced until this existed.
  std::function<void(const NodeRef& node, std::function<void()> redraw)>
      load_limits;
  // Recolours the plotted series the series section is showing. The host
  // resolves the active view's SeriesModel at call time, exactly as the command
  // handlers above are resolved, so the panel never holds a view pointer.
  std::function<void(QColor)> on_series_color_chosen;
};

// The reshell Inspector: a right-hand panel that reflects the active view's
// current selection — identity, live value + quality, and the control action —
// matching docs/product/ui-mockups/screens/substation-display.html.
//
// Selection flows in through ShowSelection(): a display element click selects a
// TimedDataSpec on the active view's SelectionModel, the main window routes the
// change here, and the panel shows the element's live value and offers control.
//
// Opt-in: the host builds this only under the reshell UX theme.
class InspectorPanel : public QWidget {
  Q_OBJECT

 public:
  explicit InspectorPanel(InspectorPanelContext context,
                          QWidget* parent = nullptr);
  ~InspectorPanel() override;

  // Reflects `selection`: empty → empty state; a data selection → the live
  // element readout. The panel copies the selection's (already-connected)
  // TimedDataSpec, so the value keeps ticking between selection changes.
  void ShowSelection(const SelectionModel& selection);

  // Clears to the empty state.
  void Clear();

  // Fills the element sections directly. This is the render primitive that
  // ShowSelection drives, and the seam the widget tests exercise.
  void ShowElement(const InspectorElementView& element);

  // Fills the alarm card for a journal-event selection. Render primitive
  // behind ShowSelection's event branch and the widget tests / capture.
  void ShowEvent(const InspectorEventView& event);

  // Shows or hides the plotted-series section of the element card. The host
  // calls this after ShowSelection with the active view's SeriesModel read out,
  // or with nullopt for a view that plots nothing — which is every view but the
  // chart, so the section is absent by default rather than empty.
  void ShowSeries(const std::optional<InspectorSeriesView>& series);

 private:
  QWidget* BuildEmptyState();
  QWidget* BuildElementView();
  QWidget* BuildEventView();
  // Re-reads spec_ (title/value/quality/updated) into the element view.
  void RefreshValue();
  // Rebuilds the limits block; hides it when the node configures no bands.
  void ShowLimits(const std::vector<InspectorLimitRow>& limits);
  // Builds the plotted-series section: the palette swatch row and the display
  // flags. Hidden until ShowSeries fills it.
  QWidget* BuildSeriesSection();
  // Rebuilds the event card's History block.
  void ShowTimeline(const std::vector<InspectorTimelineRow>& timeline);

  InspectorPanelContext context_;

  // The panel's own copy of the selected node's live spec (shares the
  // underlying TimedData); its update_handler drives RefreshValue.
  std::unique_ptr<TimedDataSpec> spec_;

  // [0] empty state, [1] element view, [2] event (alarm) card.
  QStackedWidget* stack_ = nullptr;
  QLabel* title_ = nullptr;
  QLabel* subtitle_ = nullptr;
  QLabel* value_ = nullptr;
  QLabel* quality_ = nullptr;
  QLabel* updated_ = nullptr;
  QPushButton* control_ = nullptr;
  // Describes what the Control button does. Hidden while control is
  // unavailable, where it would only advertise an action the operator cannot
  // take, directly above the reason it is blocked.
  QLabel* control_hint_ = nullptr;
  QLabel* control_reason_ = nullptr;
  // The limits block: a section header plus one row per configured band,
  // hidden wholesale when the node configures none.
  QWidget* limits_ = nullptr;
  QLabel* limits_header_ = nullptr;

  // The plotted-series block: palette swatches plus the display flags, hidden
  // wholesale unless the active view supplies a series.
  QWidget* series_ = nullptr;
  QWidget* series_swatches_ = nullptr;
  QLabel* series_own_pane_ = nullptr;
  QLabel* series_dots_ = nullptr;
  QLabel* series_stepped_ = nullptr;

  // Event-card widgets.
  QLabel* event_title_ = nullptr;
  QLabel* event_subtitle_ = nullptr;
  QLabel* event_severity_ = nullptr;
  QLabel* event_message_ = nullptr;
  QLabel* event_time_ = nullptr;
  QLabel* event_acknowledged_ = nullptr;
  QPushButton* acknowledge_ = nullptr;
  QPushButton* go_to_source_ = nullptr;
  // The History block: a section header plus one row per lifecycle step,
  // hidden wholesale when there is nothing to show.
  QWidget* timeline_ = nullptr;
};
