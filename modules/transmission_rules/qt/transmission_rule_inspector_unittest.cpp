#include "transmission_rules/qt/transmission_rule_inspector.h"

#include "aui/test/app_environment.h"
#include "model/devices_node_ids.h"
#include "node_service/test/fake_node_service.h"
#include "scada/node_id.h"

#include <gtest/gtest.h>

#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QStackedWidget>

namespace {

class TransmissionRuleInspectorTest : public ::testing::Test {
 protected:
  AppEnvironment app_env_;

  static TransmissionRuleDisplay SampleRule() {
    TransmissionRuleDisplay rule;
    rule.node_id = scada::NodeId{733, 1};
    rule.summary = QStringLiteral("Ua → 2001");
    rule.protocol = QStringLiteral("Modbus");
    rule.source_name = QStringLiteral("Ua");
    rule.signal_tag = QStringLiteral("TI");
    rule.endpoint = QStringLiteral("Retranslation");
    rule.ioa = 2001;
    return rule;
  }
};

TEST_F(TransmissionRuleInspectorTest, StartsInEmptyState) {
  TransmissionRuleInspector inspector;
  auto* stack = inspector.findChild<QStackedWidget*>();
  ASSERT_NE(stack, nullptr);
  EXPECT_EQ(stack->currentIndex(), 0);
}

TEST_F(TransmissionRuleInspectorTest, ShowRuleFillsFieldsAndTagPill) {
  TransmissionRuleInspector inspector;
  inspector.SetApplyHandler([](const scada::NodeId&, scada::Int32) {});
  inspector.ShowRuleDisplay(SampleRule());

  auto* stack = inspector.findChild<QStackedWidget*>();
  ASSERT_NE(stack, nullptr);
  EXPECT_EQ(stack->currentIndex(), 1);

  auto* pill = inspector.findChild<QLabel*>(QStringLiteral("signalTagPill"));
  ASSERT_NE(pill, nullptr);
  EXPECT_EQ(pill->text(), QStringLiteral("TI"));

  auto* ioa = inspector.findChild<QLineEdit*>(QStringLiteral("ioaEdit"));
  ASSERT_NE(ioa, nullptr);
  EXPECT_EQ(ioa->text(), QStringLiteral("2001"));

  // A freshly-shown rule is not dirty: Revert/Apply disabled.
  EXPECT_FALSE(inspector.dirty());
  auto* apply = inspector.findChild<QPushButton*>(QStringLiteral("applyRule"));
  ASSERT_NE(apply, nullptr);
  EXPECT_FALSE(apply->isEnabled());
}

TEST_F(TransmissionRuleInspectorTest, EditingIoaMarksDirtyAndApplyFiresHandler) {
  scada::NodeId applied_id;
  scada::Int32 applied_ioa = 0;
  TransmissionRuleInspector inspector;
  inspector.SetApplyHandler(
      [&](const scada::NodeId& id, scada::Int32 ioa) {
        applied_id = id;
        applied_ioa = ioa;
      });
  inspector.ShowRuleDisplay(SampleRule());

  auto* ioa = inspector.findChild<QLineEdit*>(QStringLiteral("ioaEdit"));
  ASSERT_NE(ioa, nullptr);
  ioa->setText(QStringLiteral("2005"));
  // textEdited is not emitted by setText; emit it explicitly to drive dirty.
  emit ioa->textEdited(QStringLiteral("2005"));

  EXPECT_TRUE(inspector.dirty());
  auto* apply = inspector.findChild<QPushButton*>(QStringLiteral("applyRule"));
  ASSERT_NE(apply, nullptr);
  EXPECT_TRUE(apply->isEnabled());

  apply->click();
  EXPECT_EQ(applied_id, (scada::NodeId{733, 1}));
  EXPECT_EQ(applied_ioa, 2005);
  // Applying rebases the live value, so the form is no longer dirty.
  EXPECT_FALSE(inspector.dirty());
}

TEST_F(TransmissionRuleInspectorTest, RevertRestoresLiveIoa) {
  TransmissionRuleInspector inspector;
  inspector.SetApplyHandler([](const scada::NodeId&, scada::Int32) {});
  inspector.ShowRuleDisplay(SampleRule());

  auto* ioa = inspector.findChild<QLineEdit*>(QStringLiteral("ioaEdit"));
  ASSERT_NE(ioa, nullptr);
  ioa->setText(QStringLiteral("2005"));
  emit ioa->textEdited(QStringLiteral("2005"));
  ASSERT_TRUE(inspector.dirty());

  auto* revert = inspector.findChild<QPushButton*>(QStringLiteral("revertRule"));
  ASSERT_NE(revert, nullptr);
  revert->click();

  EXPECT_EQ(ioa->text(), QStringLiteral("2001"));
  EXPECT_FALSE(inspector.dirty());
}

TEST_F(TransmissionRuleInspectorTest, WithoutHandlerIsReadOnly) {
  TransmissionRuleInspector inspector;  // no ApplyHandler
  inspector.ShowRuleDisplay(SampleRule());

  auto* ioa = inspector.findChild<QLineEdit*>(QStringLiteral("ioaEdit"));
  ASSERT_NE(ioa, nullptr);
  EXPECT_TRUE(ioa->isReadOnly());

  auto* apply = inspector.findChild<QPushButton*>(QStringLiteral("applyRule"));
  ASSERT_NE(apply, nullptr);
  EXPECT_TRUE(apply->isHidden());
}

TEST_F(TransmissionRuleInspectorTest, ClearReturnsToEmptyState) {
  TransmissionRuleInspector inspector;
  inspector.ShowRuleDisplay(SampleRule());
  inspector.Clear();
  auto* stack = inspector.findChild<QStackedWidget*>();
  ASSERT_NE(stack, nullptr);
  EXPECT_EQ(stack->currentIndex(), 0);
}

// Regression: the rule's source and address are property children reached
// through its type chain, and a selection makes none of it resident — so the
// panel rendered "— → 0" for every rule an operator selected: no source, no
// signal tag, IOA 0, a configured rule looking unconfigured. The panel now asks
// its host to load the rule and redraws when it lands.
//
// The *order* is half the fix and is what this asserts: the load is asked for
// before the type test, because IsInstanceOf walks the same unresident type
// chain and would clear the panel before any field was looked at.
TEST_F(TransmissionRuleInspectorTest, LoadsTheRuleBeforeJudgingItsType) {
  constexpr scada::NodeId kRule{733, 12};
  constexpr scada::NodeId kRuleType{900, 12};
  constexpr scada::NodeId kEndpoint{702, 12};
  constexpr scada::NodeId kSource{723, 12};

  FakeNodeService nodes;
  // Registered with no type definition: the state a rule is in before anything
  // fetched it, in which IsInstanceOf cannot recognise it.
  nodes.Add(scada::NodeState{.node_id = kRule,
                             .node_class = scada::NodeClass::Object});

  TransmissionRuleInspector inspector;
  NodeRef asked;
  std::function<void()> redraw;
  inspector.SetLoadHandler(
      [&](const NodeRef& rule, std::function<void()> done) {
        asked = rule;
        redraw = std::move(done);
      });

  inspector.ShowRule(nodes.GetNode(kRule));

  auto* stack = inspector.findChild<QStackedWidget*>();
  ASSERT_NE(stack, nullptr);
  EXPECT_EQ(stack->currentIndex(), 0);
  EXPECT_EQ(asked.node_id(), kRule)
      << "the panel judged the node's type without asking for it first";
  ASSERT_TRUE(redraw);

  // The load lands: the rule becomes recognisable and readable.
  nodes.Add(scada::NodeState{.node_id = kRuleType,
                             .node_class = scada::NodeClass::ObjectType,
                             .supertype_id =
                                 scada::devices::id::TransmissionItemType});
  nodes.Add(scada::NodeState{
      .node_id = kEndpoint,
      .node_class = scada::NodeClass::Object,
      .type_definition_id = kRuleType,
      .attributes = {.display_name = u"Retranslation"}});
  nodes.Add(scada::NodeState{.node_id = kSource,
                             .node_class = scada::NodeClass::Variable,
                             .type_definition_id = kRuleType,
                             .attributes = {.display_name = u"Ua"}});
  nodes.Add(scada::NodeState{
      .node_id = kRule,
      .node_class = scada::NodeClass::Object,
      .type_definition_id = kRuleType,
      .parent_id = kEndpoint,
      .reference_type_id = scada::id::Organizes,
      .properties = {
          {scada::devices::id::TransmissionItemType_SourceNode, kSource},
          {scada::devices::id::TransmissionItemType_Address,
           static_cast<scada::Int32>(2001)}}});
  redraw();

  EXPECT_EQ(stack->currentIndex(), 1);
  auto* ioa = inspector.findChild<QLineEdit*>();
  ASSERT_NE(ioa, nullptr);
  EXPECT_EQ(ioa->text(), QStringLiteral("2001"));
}

// A reply for a rule the operator has moved off must not repaint the card.
TEST_F(TransmissionRuleInspectorTest, StaleLoadReplyIsIgnored) {
  constexpr scada::NodeId kFirst{733, 12};
  constexpr scada::NodeId kSecond{734, 12};

  FakeNodeService nodes;
  for (const scada::NodeId& id : {kFirst, kSecond}) {
    nodes.Add(scada::NodeState{.node_id = id,
                               .node_class = scada::NodeClass::Object});
  }

  TransmissionRuleInspector inspector;
  std::function<void()> first_redraw;
  int loads = 0;
  inspector.SetLoadHandler(
      [&](const NodeRef& rule, std::function<void()> done) {
        ++loads;
        if (rule.node_id() == kFirst)
          first_redraw = std::move(done);
      });

  inspector.ShowRule(nodes.GetNode(kFirst));
  inspector.ShowRule(nodes.GetNode(kSecond));
  EXPECT_EQ(loads, 2);

  ASSERT_TRUE(first_redraw);
  first_redraw();

  // Still the second rule's (empty) state, and no third load was started —
  // a stale reply that re-entered ShowRule would have asked again.
  EXPECT_EQ(loads, 2);
}

}  // namespace
