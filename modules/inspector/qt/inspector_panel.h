#pragma once

#include <QString>
#include <QWidget>

#include <functional>
#include <memory>

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

// Action wiring for the Inspector panel.
struct InspectorPanelContext {
  // Triggers the selection-scoped control/write command — the existing
  // two-stage confirm flow. Wired by the host to ExecuteCommand(ID_WRITE).
  std::function<void()> on_control;
  // Whether the control command is currently enabled for the active selection.
  std::function<bool()> is_control_enabled;
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
  void ShowElement(const QString& title,
                   const QString& node_id_text,
                   const QString& value_text,
                   InspectorQualityBand quality,
                   const QString& updated_text,
                   bool controllable);

 private:
  QWidget* BuildEmptyState();
  QWidget* BuildElementView();
  // Re-reads spec_ (title/value/quality/updated) into the element view.
  void RefreshValue();

  InspectorPanelContext context_;

  // The panel's own copy of the selected node's live spec (shares the
  // underlying TimedData); its update_handler drives RefreshValue.
  std::unique_ptr<TimedDataSpec> spec_;

  QStackedWidget* stack_ = nullptr;  // [0] empty state, [1] element view
  QLabel* title_ = nullptr;
  QLabel* subtitle_ = nullptr;
  QLabel* value_ = nullptr;
  QLabel* quality_ = nullptr;
  QLabel* updated_ = nullptr;
  QPushButton* control_ = nullptr;
};
