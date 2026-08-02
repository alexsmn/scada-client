#include "transmission_rules/qt/transmission_rule_inspector.h"

#include "aui/test/app_environment.h"
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

}  // namespace
