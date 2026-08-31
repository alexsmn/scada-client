#pragma once

#include "node_service/node_ref.h"
#include "scada/basic_types.h"
#include "scada/node_id.h"

#include <QString>
#include <QWidget>

#include <functional>

class QLabel;
class QLineEdit;
class QPushButton;
class QStackedWidget;

// One rendered rule, the seam the widget tests drive without a node service.
struct TransmissionRuleDisplay {
  scada::NodeId node_id;  // the rule node Apply writes to
  QString summary;        // "Ua → 2001"
  QString protocol;       // "Modbus"
  QString source_name;    // "Ua"
  QString signal_tag;     // "TI" (or empty)
  QString endpoint;       // destination device name
  scada::Int32 ioa = 0;
};

// The reshell transmission rule editor — the right region of
// docs/product/ui-mockups/screens/transmission-rules.html. For a selected
// transmission item it shows the source signal, the destination endpoint and
// IOA (editable, staged behind Revert/Apply), and the protocol.
//
// Scope: the mockup's Trigger / Transform / Status / counters are server-side
// device-driver concepts that never reach the client — the client node model
// carries only the source link and the destination address — so they are
// intentionally not surfaced here (see modules/transmission_rules/README seam
// in transmission_rule.h).
//
// Selection flows in through ShowRule(): the host routes a
// TransmissionItemType-node selection here.
//
// Opt-in: construct this only under the reshell UX theme (see
// MakeTransmissionRuleInspector).
class TransmissionRuleInspector : public QWidget {
  Q_OBJECT

 public:
  // Fired on Apply with the rule node id and the edited IOA; the host routes it
  // to the write path (TaskManager::PostUpdateTask).
  using ApplyHandler = std::function<void(const scada::NodeId&, scada::Int32)>;

  // Asked to make `transmission` readable, then to call `redraw`.
  //
  // The inspector asks rather than fetching: a rule reads in two hops and a
  // selection makes neither resident (see FetchTransmissionRule in
  // modules/transmission_rules/transmission_rule_fetch.h), and the fetch needs
  // the host's executor. Unwired, ShowRule renders whatever happens to be
  // resident — which for a rule the operator just selected is a source-less
  // rule at IOA 0.
  using LoadHandler =
      std::function<void(const NodeRef&, std::function<void()> redraw)>;

  explicit TransmissionRuleInspector(QWidget* parent = nullptr);
  ~TransmissionRuleInspector() override;

  void SetApplyHandler(ApplyHandler handler);
  void SetLoadHandler(LoadHandler handler);

  // Reflects `transmission` (expected to be a TransmissionItemType instance):
  // reads its source, endpoint, protocol and IOA. A null / non-transmission
  // node clears.
  void ShowRule(const NodeRef& transmission);

  // Clears to the empty state.
  void Clear();

  // Render primitive driven by ShowRule — exercised directly by the tests.
  void ShowRuleDisplay(const TransmissionRuleDisplay& rule);

  // True when the IOA field differs from the last-shown live value.
  bool dirty() const;

 private:
  QWidget* BuildEmptyState();
  QWidget* BuildContent();
  void OnIoaEdited();
  void OnRevert();
  void OnApply();
  void UpdateDirtyState();

  QStackedWidget* stack_ = nullptr;  // [0] empty state, [1] content.
  QLabel* summary_ = nullptr;
  QLabel* protocol_ = nullptr;
  QLabel* source_ = nullptr;
  QLabel* signal_tag_ = nullptr;
  QLabel* endpoint_ = nullptr;
  QLineEdit* ioa_edit_ = nullptr;
  QPushButton* revert_ = nullptr;
  QPushButton* apply_ = nullptr;

  scada::NodeId rule_id_;
  scada::Int32 live_ioa_ = 0;
  ApplyHandler apply_handler_;
  LoadHandler load_handler_;
  // The rule the last load was asked for, so a reply that arrives after the
  // operator moved on is dropped instead of redrawing another rule's card.
  scada::NodeId loading_id_;
};

// Builds a TransmissionRuleInspector. Ownership transfers to the caller.
TransmissionRuleInspector* MakeTransmissionRuleInspector();
