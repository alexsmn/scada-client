#pragma once

#include "device_diagnostics/device_link_state.h"
#include "device_diagnostics/protocol_diagnostics.h"
#include "node_service/node_ref.h"

#include <QString>
#include <QWidget>

#include <functional>
#include <memory>
#include <optional>
#include <vector>

class TimedDataService;
class TimedDataSpec;
class QFrame;
class QLabel;
class QPushButton;
class QStackedWidget;
class QVBoxLayout;

// One button in the diagnostics panel's Actions section — wired by the host to
// a device command (Metrics trend / Open log) or supplied per device by the
// protocol registry (Reconnect now). `execute` runs it; `is_enabled` gates the
// button (empty → always enabled).
struct DiagnosticAction {
  std::u16string label;
  std::function<void()> execute;
  std::function<bool()> is_enabled;
  // Rendered under the button while it is disabled. A disabled control must
  // state a reason an operator can act on, so leaving this empty means the
  // action is one whose unavailability is self-evident from the selection —
  // never "we did not implement it".
  std::u16string disabled_reason;
};

// Action wiring for the diagnostics panel.
struct DeviceDiagnosticsPanelContext {
  // Actions that apply to any device, wired once by the host.
  std::vector<DiagnosticAction> actions;

  // Calls an OPC UA Method on the selected device's PARENT LINK — the seam for
  // the protocol registry's link action (ADR 0007's Reconnect). The panel
  // resolves which link and which method; the host owns the call path, error
  // reporting and progress.
  //
  // Unset means the host offers no method-call path, and the panel then draws
  // NO link-action button at all. That is deliberate: "this build cannot call
  // methods" is not a reason an operator can act on, so it is not worth a
  // disabled button. Lacking the Call permission is different — that is
  // explained, below.
  std::function<void(const NodeRef& link, const scada::NodeId& method_id)>
      call_link_method;

  // Whether this session holds the OPC UA Call permission (PermissionType.Call,
  // Part 3 §8.55). This is the client-side reading of the method node's
  // UserExecutable attribute (Part 3 §5.7.1) — the server computes the same
  // predicate from the same session rights, and remains the authority: a call
  // that slips past this answers Bad_UserAccessDenied. Unset means "assume
  // yes".
  std::function<bool()> can_call;
};

// One diagnostic reading rendered as a "Label   Value" row.
struct DeviceDiagnosticRow {
  QString label;
  QString value;
  bool bad = false;  // renders the value in the bad/alarm colour.
  // A section heading rather than a reading: rendered as a subdued caption with
  // no value. Kept as a row so the render primitive stays one flat list and the
  // widget tests that drive it directly are unaffected.
  bool heading = false;
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

  // The registry's link action for the selected device, bound to `link` — the
  // panel's decision procedure for whether that button exists at all, exposed
  // so it can be exercised without a live node service.
  //
  // Returns nullopt when the protocol declares no action, when the device has
  // no link (there is nothing to act on), or when the host wired no
  // method-call path. All three mean "no button", never "dead button".
  std::optional<DiagnosticAction> MakeLinkAction(
      const ProtocolLinkAction& action,
      const NodeRef& link);

 private:
  QWidget* BuildEmptyState();
  QWidget* BuildContent();
  // Recomputes the band + rows from the live specs and re-renders.
  void RefreshFromSpecs();
  // Re-queries each action's is_enabled and updates its button and its
  // disabled-reason line.
  void RefreshActions();
  // (Re)creates the action buttons for `link_actions_` followed by
  // `context_.actions`. Called whenever the selection changes, because the
  // link action is per device: it exists only for a registered protocol whose
  // parent link resolved.
  void RebuildActionButtons();

  DeviceDiagnosticsPanelContext context_;

  // One live counter reading resolved from the device. The spec drives live
  // updates; `node` supplies the Value-attribute snapshot used when the
  // monitored-item value has not been delivered yet (e.g. right after selection,
  // or in the headless capture).
  struct Reading {
    QString label;
    NodeRef node;
    std::unique_ptr<TimedDataSpec> spec;
    // How the raw value is turned into text; counters are plain numbers, a link
    // state is a word, a t1 flag is a condition.
    ProtocolValueShape shape = ProtocolValueShape::kCount;
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
  // The selected device's PARENT LINK readings, when its protocol is registered
  // (ADR 0007). Empty for an unregistered protocol or a device with no link, so
  // the section is omitted rather than drawn empty.
  std::vector<Reading> link_readings_;
  QString link_section_label_;
  // The selected device's link action, when its protocol declares one. Rebuilt
  // per selection, and drawn BEFORE the host's device-wide actions so the
  // Actions section reads in the mockup's order.
  std::vector<DiagnosticAction> link_actions_;

  QStackedWidget* stack_ = nullptr;  // [0] empty state, [1] content.
  QLabel* name_ = nullptr;
  QLabel* type_ = nullptr;
  QFrame* hero_ = nullptr;
  QLabel* hero_status_ = nullptr;
  QLabel* hero_detail_ = nullptr;
  QVBoxLayout* rows_layout_ = nullptr;  // owns the current DeviceDiagnosticRow widgets.
  QVBoxLayout* actions_layout_ = nullptr;  // owns the current action widgets.
  // One entry per rendered action, in the order link_actions_ then
  // context_.actions. The reason label is null when the action supplies none.
  struct ActionWidgets {
    QPushButton* button = nullptr;
    QLabel* reason = nullptr;
  };
  std::vector<ActionWidgets> action_widgets_;
};

// Builds a DeviceDiagnosticsPanel under the reshell UX theme
// (scada::aui::GetSeverityTheme() != SeverityTheme::kLegacy); returns nullptr in
// the legacy look so the host adds no diagnostics dock. Ownership transfers to
// the caller.
DeviceDiagnosticsPanel* MakeDeviceDiagnosticsPanel(
    DeviceDiagnosticsPanelContext context);
