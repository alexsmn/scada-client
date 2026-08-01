#pragma once

#include "device_diagnostics/device_link_state.h"
#include "node_service/node_ref.h"

#include <QString>
#include <QWidget>

#include <functional>
#include <memory>
#include <vector>

class TimedDataService;
class TimedDataSpec;
class QFrame;
class QLabel;
class QPushButton;
class QStackedWidget;
class QVBoxLayout;

// One button in the diagnostics panel's Actions section — wired by the host to a
// device command (Metrics trend / Reconnect / Open log). `execute` runs it;
// `is_enabled` gates the button (empty → always enabled).
struct DiagnosticAction {
  std::u16string label;
  std::function<void()> execute;
  std::function<bool()> is_enabled;
};

// Action wiring for the diagnostics panel.
struct DeviceDiagnosticsPanelContext {
  std::vector<DiagnosticAction> actions;
};

// One diagnostic reading rendered as a "Label   Value" row.
struct DeviceDiagnosticRow {
  QString label;
  QString value;
  bool bad = false;  // renders the value in the bad/alarm colour.
};

// The reshell Device Diagnostics inspector — the right region of
// docs/product/ui-mockups/screens/config-workbench.html. For a selected device
// it shows a link-status hero (up / down / disabled), live traffic and polling
// counters, and a Metrics-trend action.
//
// Selection flows in through ShowDevice(): the host routes a device-node
// selection here; the panel resolves the device's diagnostic child variables
// (Online / Enabled / MessagesIn / …) and connects a live spec per reading, so
// the readings tick while the device stays selected.
//
// Opt-in: construct this only under the reshell UX theme (see
// MakeDeviceDiagnosticsPanel).
class DeviceDiagnosticsPanel : public QWidget {
  Q_OBJECT

 public:
  explicit DeviceDiagnosticsPanel(DeviceDiagnosticsPanelContext context,
                                  QWidget* parent = nullptr);
  ~DeviceDiagnosticsPanel() override;

  // Reflects `device` (expected to be a DeviceType instance): resolves its
  // diagnostic child variables, connects a live spec per reading through
  // `timed_data_service`, and fills the hero + rows. The readings keep ticking
  // until the next ShowDevice/Clear.
  void ShowDevice(const NodeRef& device, TimedDataService& timed_data_service);

  // Clears to the empty state and drops the live specs.
  void Clear();

  // Render primitive that ShowDevice drives — the seam the widget tests
  // exercise without a node service.
  void ShowDiagnostics(const QString& name,
                       const QString& type_label,
                       DeviceLinkBand band,
                       const QString& band_detail,
                       const std::vector<DeviceDiagnosticRow>& rows);

 private:
  QWidget* BuildEmptyState();
  QWidget* BuildContent();
  // Recomputes the band + rows from the live specs and re-renders.
  void RefreshFromSpecs();
  // Re-queries each action's is_enabled and updates its button.
  void RefreshActions();

  DeviceDiagnosticsPanelContext context_;

  // One live counter reading resolved from the device. The spec drives live
  // updates; `node` supplies the Value-attribute snapshot used when the
  // monitored-item value has not been delivered yet (e.g. right after selection,
  // or in the headless capture).
  struct Reading {
    QString label;
    NodeRef node;
    std::unique_ptr<TimedDataSpec> spec;
  };

  QString device_name_;
  QString device_type_;
  // The booleans that drive the hero band; the nodes are null when the device
  // omits them.
  NodeRef online_node_;
  NodeRef enabled_node_;
  std::unique_ptr<TimedDataSpec> online_spec_;
  std::unique_ptr<TimedDataSpec> enabled_spec_;
  std::vector<Reading> readings_;

  QStackedWidget* stack_ = nullptr;  // [0] empty state, [1] content.
  QLabel* name_ = nullptr;
  QLabel* type_ = nullptr;
  QFrame* hero_ = nullptr;
  QLabel* hero_status_ = nullptr;
  QLabel* hero_detail_ = nullptr;
  QVBoxLayout* rows_layout_ = nullptr;  // owns the current DeviceDiagnosticRow widgets.
  // One button per context action, parallel to context_.actions.
  std::vector<QPushButton*> action_buttons_;
};

// Builds a DeviceDiagnosticsPanel under the reshell UX theme
// (scada::aui::GetSeverityTheme() != SeverityTheme::kLegacy); returns nullptr in
// the legacy look so the host adds no diagnostics dock. Ownership transfers to
// the caller.
DeviceDiagnosticsPanel* MakeDeviceDiagnosticsPanel(
    DeviceDiagnosticsPanelContext context);
