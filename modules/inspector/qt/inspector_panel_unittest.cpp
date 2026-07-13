#include "inspector/qt/inspector_panel.h"

#include "aui/test/app_environment.h"
#include "scada/qualifier.h"

#include <gtest/gtest.h>

#include <QLabel>
#include <QPushButton>
#include <QStackedWidget>

namespace {

// The pure quality-band mapping needs no QApplication.
TEST(InspectorQualityBandTest, GoodQualifierIsGood) {
  EXPECT_EQ(InspectorQualityBandFor(scada::Qualifier{}),
            InspectorQualityBand::kGood);
}

TEST(InspectorQualityBandTest, BadQualifierIsBad) {
  const scada::Qualifier bad{scada::Qualifier::BAD};
  EXPECT_EQ(InspectorQualityBandFor(bad), InspectorQualityBand::kBad);
}

class InspectorPanelTest : public ::testing::Test {
 protected:
  // Qt requires a QApplication before any QWidget; destroyed with the fixture.
  AppEnvironment app_env_;
};

TEST_F(InspectorPanelTest, StartsInEmptyState) {
  InspectorPanel panel{InspectorPanelContext{}};
  auto* stack =
      panel.findChild<QStackedWidget*>(QStringLiteral("inspectorStack"));
  ASSERT_NE(stack, nullptr);
  EXPECT_EQ(stack->currentIndex(), 0);
}

TEST_F(InspectorPanelTest, ShowElementFillsReadoutAndControl) {
  bool control_enabled = true;
  InspectorPanel panel{InspectorPanelContext{
      .is_control_enabled = [&] { return control_enabled; }}};

  panel.ShowElement(QStringLiteral("Q1"), QStringLiteral("ns=2;s=North.Q1"),
                    QStringLiteral("195.7 A"), InspectorQualityBand::kGood,
                    QStringLiteral("21:53:58"), /*controllable=*/true);

  auto* stack =
      panel.findChild<QStackedWidget*>(QStringLiteral("inspectorStack"));
  ASSERT_NE(stack, nullptr);
  EXPECT_EQ(stack->currentIndex(), 1);

  auto* value = panel.findChild<QLabel*>(QStringLiteral("inspectorValue"));
  ASSERT_NE(value, nullptr);
  EXPECT_EQ(value->text(), QStringLiteral("195.7 A"));

  auto* control =
      panel.findChild<QPushButton*>(QStringLiteral("inspectorControl"));
  ASSERT_NE(control, nullptr);
  EXPECT_TRUE(control->isEnabled());
}

TEST_F(InspectorPanelTest, ControlDisabledWhenNotControllable) {
  InspectorPanel panel{InspectorPanelContext{}};
  panel.ShowElement(QStringLiteral("Bus"), QString{}, QStringLiteral("115 kV"),
                    InspectorQualityBand::kGood, QStringLiteral("21:53"),
                    /*controllable=*/false);
  auto* control =
      panel.findChild<QPushButton*>(QStringLiteral("inspectorControl"));
  ASSERT_NE(control, nullptr);
  EXPECT_FALSE(control->isEnabled());
}

TEST_F(InspectorPanelTest, ClearReturnsToEmptyState) {
  InspectorPanel panel{InspectorPanelContext{}};
  panel.ShowElement(QStringLiteral("Q1"), QString{}, QStringLiteral("1"),
                    InspectorQualityBand::kGood, QString{}, false);
  panel.Clear();
  auto* stack =
      panel.findChild<QStackedWidget*>(QStringLiteral("inspectorStack"));
  ASSERT_NE(stack, nullptr);
  EXPECT_EQ(stack->currentIndex(), 0);
}

}  // namespace
