#include "hardware_tree_model.h"

#include "aui/severity_colors.h"
#include "configuration/devices/device_state_color.h"
#include "configuration/tree/node_service_tree_impl.h"
#include "model/devices_node_ids.h"
#include "node_service/node_ref.h"
#include "node_service/node_service.h"
#include "node_service/node_util.h"
#include "services/device_state_notifier.h"
#include "transmission_rules/transmission_rule.h"
#include "transmission_rules/transmission_rule_fetch.h"

namespace {

bool IsHardwareDeviceNode(const NodeRef& node) {
  return IsInstanceOf(node, scada::devices::id::DeviceType) ||
         IsInstanceOf(node, scada::devices::id::ModbusDeviceType) ||
         IsInstanceOf(node, scada::devices::id::Iec60870DeviceType) ||
         IsInstanceOf(node, scada::devices::id::Iec61850DeviceType);
}

// Compared by id rather than with IsInstanceOf, for the reason the model's
// `leaf_type_definition_ids_` gives: a rule is typed with its per-protocol
// subtype, and walking up to TransmissionItemType reads type nodes nothing has
// fetched when the row is created.
bool IsTransmissionRuleNode(const NodeRef& node) {
  const scada::NodeId type_id = node.type_definition().node_id();
  return type_id == scada::devices::id::TransmissionItemType ||
         type_id == scada::devices::id::ModbusTransmissionItemType ||
         type_id == scada::devices::id::Iec60870TransmissionItemType ||
         type_id == scada::devices::id::Iec61850TransmissionItemType;
}

// The source signal a rule transmits. `SourceNode` holds a NodeId *value*
// rather than a reference, and reads null until FetchTransmissionRule has made
// the rule's property children and type chain resident.
scada::NodeId RuleSourceId(const NodeRef& rule) {
  return rule[scada::devices::id::TransmissionItemType_SourceNode]
      .value()
      .get_or(scada::NodeId{});
}

// "Ua → 2001" — the line the rules grid and both clients' rule inspectors
// already head a rule with, so a rule reads the same everywhere.
std::u16string RuleLabel(const NodeRef& rule) {
  const scada::NodeId source_id = RuleSourceId(rule);
  const NodeRef source =
      source_id.is_null() ? NodeRef{} : rule.service()->GetNode(source_id);
  const scada::Int32 ioa =
      rule[scada::devices::id::TransmissionItemType_Address]
          .value()
          .get_or<scada::Int32>(0);
  return TransmissionRuleSummary(
      source ? ToString16(source.display_name()) : std::u16string{}, ioa);
}

}  // namespace

// HardwareTreeModel::DeviceTreeNode

class HardwareTreeModel::DeviceTreeNode : public ConfigurationTreeNode {
 public:
  DeviceTreeNode(HardwareTreeModel& model,
                 scada::NodeId reference_type_id,
                 bool forward_reference,
                 const NodeRef& node);

  // ConfigurationTreeNode
  virtual void OnModelChanged() override;

  // TreeNode
  virtual std::u16string GetText(int column_id) const override;
  virtual int GetIcon() const override;

  // The device's connection state: the live DeviceStateNotifier value, falling
  // back to the node's Value-attribute snapshot (DeviceStateFromNode) while the
  // notifier still reads Unknown (right after load, and in the headless
  // capture, where monitored values are not delivered).
  DeviceState device_state() const;

  std::optional<DeviceState> GetDeviceStateForTesting() const;

 private:
  void UpdateNotifier();

  // A transmission rule carries no display text of its own: the tier
  // synthesises `<namespace name>.<id>` for every rule row lacking one, which
  // names the row without identifying the rule. The label that does identify
  // it needs the rule's two properties and its source's display name, none of
  // which the tree has made resident, and GetText runs on every paint — so
  // the reads are requested once here, and the row repaints when they land.
  void UpdateRuleLabel();

  static Awaitable<void> ResolveRuleAsync(std::weak_ptr<void> lifetime_token,
                                          DeviceTreeNode* tree_node,
                                          NodeRef rule);

  std::unique_ptr<DeviceStateNotifier> device_state_notifier_;

  // Set once a rule's reads have been requested, and the source they were
  // requested for — so a model change that re-points the rule at another
  // source asks again, while the change the fetch itself provokes does not.
  bool rule_requested_ = false;
  scada::NodeId rule_source_id_;
  // True once the reads have landed; until then the row keeps the base text,
  // "[Loading]" suffix included.
  bool rule_resolved_ = false;

  // Outlives nothing: a reply that lands after the row was destroyed (the
  // branch collapsed and re-fetched, or the model torn down) finds it expired.
  std::shared_ptr<void> lifetime_token_ = std::make_shared<int>(0);
};

HardwareTreeModel::DeviceTreeNode::DeviceTreeNode(
    HardwareTreeModel& model,
    scada::NodeId reference_type_id,
    bool forward_reference,
    const NodeRef& node)
    : ConfigurationTreeNode{model, std::move(reference_type_id),
                            forward_reference, node} {
  UpdateNotifier();
  UpdateRuleLabel();
}

std::u16string HardwareTreeModel::DeviceTreeNode::GetText(int column_id) const {
  if (rule_resolved_)
    return RuleLabel(node());
  return ConfigurationTreeNode::GetText(column_id);
}

int HardwareTreeModel::DeviceTreeNode::GetIcon() const {
  if (IsHardwareDeviceNode(node())) {
    switch (device_state()) {
      case DeviceState::Disabled:
        return IMAGE_DEVICE_DISABLED;
      case DeviceState::Offline:
        return IMAGE_DEVICE_STOPPED;
      case DeviceState::Online:
        return IMAGE_DEVICE_RUNNING;
      case DeviceState::Unknown:
      default:
        return IMAGE_DEVICE;
    }
  }

  return ConfigurationTreeNode::GetIcon();
}

DeviceState HardwareTreeModel::DeviceTreeNode::device_state() const {
  if (!IsHardwareDeviceNode(node()))
    return DeviceState::Unknown;
  DeviceState state = device_state_notifier_
                          ? device_state_notifier_->device_state()
                          : DeviceState::Unknown;
  if (state == DeviceState::Unknown)
    state = DeviceStateFromNode(node());
  return state;
}

std::optional<DeviceState>
HardwareTreeModel::DeviceTreeNode::GetDeviceStateForTesting() const {
  if (!device_state_notifier_)
    return std::nullopt;
  return device_state_notifier_->device_state();
}

void HardwareTreeModel::DeviceTreeNode::OnModelChanged() {
  ConfigurationTreeNode::OnModelChanged();
  UpdateNotifier();
  UpdateRuleLabel();
}

void HardwareTreeModel::DeviceTreeNode::UpdateNotifier() {
  if (!device_state_notifier_ && node().fetched() &&
      IsHardwareDeviceNode(node())) {
    auto& model = static_cast<HardwareTreeModel&>(this->model());
    device_state_notifier_ = std::make_unique<DeviceStateNotifier>(
        model.timed_data_service(), this->node(), [this] { Changed(); });
  }
}

void HardwareTreeModel::DeviceTreeNode::UpdateRuleLabel() {
  if (!node().fetched() || !IsTransmissionRuleNode(node()))
    return;
  if (rule_requested_ && RuleSourceId(node()) == rule_source_id_)
    return;

  rule_requested_ = true;
  rule_source_id_ = RuleSourceId(node());
  CoSpawn(model().executor(),
          [lifetime_token = std::weak_ptr<void>{lifetime_token_},
           tree_node = this, rule = node()]() mutable -> Awaitable<void> {
            co_await ResolveRuleAsync(std::move(lifetime_token), tree_node,
                                      std::move(rule));
          });
}

// static
Awaitable<void> HardwareTreeModel::DeviceTreeNode::ResolveRuleAsync(
    std::weak_ptr<void> lifetime_token,
    DeviceTreeNode* tree_node,
    NodeRef rule) {
  co_await FetchTransmissionRule(rule);

  if (lifetime_token.expired())
    co_return;

  // Record the source the fetch actually resolved, so the model change it
  // provokes reads as settled in UpdateRuleLabel rather than as a re-point.
  tree_node->rule_source_id_ = RuleSourceId(rule);
  tree_node->rule_resolved_ = true;
  tree_node->Changed();
}

// HardwareTreeModel

HardwareTreeModel::HardwareTreeModel(HardwareTreeModelContext&& context)
    : ConfigurationTreeModel{::ConfigurationTreeModelContext{
          .executor_ = context.executor_,
          .node_service_tree_ =
              context.node_service_tree_factory_(NodeServiceTreeImplContext{
                  .executor_ = context.executor_,
                  .node_service_ = context.node_service_,
                  .root_node_ = context.node_service_.GetNode(
                      scada::devices::id::Devices),
                  .reference_filter_ = {{scada::id::Organizes, true},
                                        {scada::id::HasComponent, true}},
                  .type_definition_ids_ =
                      {scada::devices::id::DeviceType,
                       scada::devices::id::LinkType,
                       scada::devices::id::ModbusLinkType,
                       scada::devices::id::ModbusDeviceType,
                       scada::devices::id::Iec60870LinkType,
                       scada::devices::id::Iec60870DeviceType,
                       scada::devices::id::Iec61850DeviceType,
                       scada::devices::id::Iec61850LogicalNodeType,
                       scada::devices::id::Iec61850ConfigurableObjectType,
                       scada::devices::id::Iec61850DataVariableType,
                       scada::devices::id::Iec61850ControlObjectType,
                       scada::devices::id::TransmissionItemType},
                  // A transmission rule is a row, never a branch. Its only
                  // children are the `Address` and `SourceNode` property
                  // instances, which attach by HasProperty — not in this
                  // tree's reference filter, and `PropertyType` is not in the
                  // type list above either — so a rule's expander has always
                  // opened onto nothing. The parameter form is where those two
                  // belong, which is the same reasoning that makes DataItemType
                  // a leaf in ObjectTreeModel and FileType one in the
                  // filesystem tree.
                  //
                  // All four ids, not just the base type: `IsInstanceOf` walks
                  // the supertype chain, and a chain hop reads a *type* node
                  // that nothing has fetched when the row is created — see
                  // `NodeServiceTreeImpl::HasChildren`, whose comment records
                  // the expander that appeared and then vanished a round trip
                  // later. A rule instance is typed with its per-protocol
                  // subtype, so naming the subtypes makes the comparison match
                  // at the first iteration, with no fetch required.
                  .leaf_type_definition_ids_ =
                      {scada::devices::id::TransmissionItemType,
                       scada::devices::id::ModbusTransmissionItemType,
                       scada::devices::id::Iec60870TransmissionItemType,
                       scada::devices::id::Iec61850TransmissionItemType}}),
      }},
      timed_data_service_{context.timed_data_service_} {}

HardwareTreeModel::~HardwareTreeModel() {}

std::optional<DeviceState> HardwareTreeModel::GetDeviceStateForTesting(
    void* tree_node) const {
  auto* device_tree_node = dynamic_cast<DeviceTreeNode*>(
      static_cast<ConfigurationTreeNode*>(tree_node));
  return device_tree_node ? device_tree_node->GetDeviceStateForTesting()
                          : std::nullopt;
}

std::optional<scada::aui::Color> HardwareTreeModel::GetStatusColor(
    void* tree_node) {
  auto* device_tree_node = dynamic_cast<DeviceTreeNode*>(
      static_cast<ConfigurationTreeNode*>(tree_node));
  if (!device_tree_node)
    return std::nullopt;
  std::optional<scada::aui::Quality> quality =
      DeviceStateQuality(device_tree_node->device_state());
  if (!quality)
    return std::nullopt;  // Unknown state: no dot.
  return scada::aui::QualityColor(*quality);
}

std::unique_ptr<ConfigurationTreeNode> HardwareTreeModel::CreateTreeNode(
    const scada::NodeId& reference_type_id,
    bool forward_reference,
    const NodeRef& node) {
  return std::make_unique<DeviceTreeNode>(*this, reference_type_id,
                                          forward_reference, node);
}
