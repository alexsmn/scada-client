#pragma once

#include <QString>
#include <QWidget>

#include <functional>
#include <memory>
#include <vector>

namespace scada {
class NodeId;
class Qualifier;
}  // namespace scada
class SelectionModel;
class TimedDataSpec;
class QLabel;
class QPushButton;
class QStackedWidget;

// The quality band for the Inspector's state-hero pill. A pure mapping so it
// can be unit-tested without a running QApplication.
enum class InspectorQualityBand { kGood, kBad };
InspectorQualityBand InspectorQualityBandFor(const scada::Qualifier& qualifier);

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
  InspectorQualityBand quality = InspectorQualityBand::kGood;
  QString updated_text;
  // The node's configured limit bands, most severe first. Empty when the node
  // configures none, which hides the limits block entirely.
  std::vector<InspectorLimitRow> limits;
  bool controllable = false;
  // Why control is unavailable, shown under the disabled Control button.
  // Empty when control is available, or when the reason is not known.
  QString control_reason;
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
};

// The reshell Inspector: a right-hand panel that reflects the active view's
// current selection — identity, live value + quality, and the control action —
// matching client/docs/ui-mockups/screens/substation-display.html.
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

 private:
  QWidget* BuildEmptyState();
  QWidget* BuildElementView();
  QWidget* BuildEventView();
  // Re-reads spec_ (title/value/quality/updated) into the element view.
  void RefreshValue();
  // Rebuilds the limits block; hides it when the node configures no bands.
  void ShowLimits(const std::vector<InspectorLimitRow>& limits);
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
  QLabel* control_reason_ = nullptr;
  // The limits block: a section header plus one row per configured band,
  // hidden wholesale when the node configures none.
  QWidget* limits_ = nullptr;
  QLabel* limits_header_ = nullptr;

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
