#include "transmission_rules/qt/transmission_rule_selection_panel.h"

#include "aui/translation.h"
#include "base/awaitable.h"
#include "controller/qt/selection_panel_registry.h"
#include "controller/selection_model.h"
#include "model/devices_node_ids.h"
#include "node_service/node_util.h"
#include "transmission_rules/qt/transmission_rule_inspector.h"
#include "transmission_rules/transmission_rule_fetch.h"

#include <memory>
#include <string>
#include <utility>

namespace {

// Adapts TransmissionRuleInspector to the shell's selection-panel contract.
class TransmissionRuleSelectionPanel final : public SelectionPanel {
 public:
  explicit TransmissionRuleSelectionPanel(TransmissionRuleInspector& inspector)
      : inspector_{inspector} {}

  std::string object_name() const override { return "TransmissionRuleDock"; }
  std::u16string title() const override {
    return Translate("Transmission rule");
  }
  QWidget& widget() override { return inspector_; }

  void ShowSelection(const SelectionModel* selection) override {
    // A single transmission-item selection fills the rule inspector; anything
    // else clears.
    if (selection && !selection->empty() && !selection->multiple() &&
        IsInstanceOf(selection->node(),
                     scada::devices::id::TransmissionItemType)) {
      inspector_.ShowRule(selection->node());
    } else {
      inspector_.Clear();
    }
  }

 private:
  TransmissionRuleInspector& inspector_;
};

std::unique_ptr<SelectionPanel> CreatePanel(
    const SelectionPanelContext& context) {
  TransmissionRuleInspector* inspector = MakeTransmissionRuleInspector();
  if (!inspector)
    return nullptr;

  // Same bargain as the Inspector's limit bands: the panel reads a rule the
  // selection has not made resident — here in two hops, the second one a
  // NodeId property naming a peer node — so the shell's executor runs the
  // fetch and the panel asks for it.
  inspector->SetLoadHandler([executor = context.executor](
                                const NodeRef& rule,
                                std::function<void()> redraw) {
    CoSpawn(executor, [rule, redraw = std::move(redraw)]() -> Awaitable<void> {
      co_await FetchTransmissionRule(rule);
      redraw();
    });
  });
  // No ApplyHandler is wired: the shell has no TaskManager, so the panel
  // presents the rule read-only. Editing rides the existing transmission
  // grid, which writes SourceAddress through its own TaskManager.
  return std::make_unique<TransmissionRuleSelectionPanel>(*inspector);
}

}  // namespace

void RegisterTransmissionRuleSelectionPanel(SelectionPanelRegistry& registry) {
  registry.Register(&CreatePanel);
}
