#include "inspector/qt/inspector_panel.h"

#include "aui/test/app_environment.h"
#include "base/utf_convert.h"
#include "controller/selection_model.h"
#include "scada/qualifier.h"
#include "timed_data/base_timed_data.h"
#include "timed_data/timed_data_service.h"
#include "timed_data/timed_data_spec.h"

#include <gtest/gtest.h>

#include <QLabel>
#include <QPushButton>
#include <QStackedWidget>

#include <memory>
#include <string>

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

// A minimal live datum identified by its formula alone (no backing node) —
// the shape of a Table expression row.
class FormulaTimedData : public BaseTimedData {
 public:
  explicit FormulaTimedData(std::string formula)
      : formula_{std::move(formula)} {}

  virtual std::string GetFormula(bool aliases) const override {
    return formula_;
  }
  virtual scada::LocalizedText GetTitle() const override {
    return UtfConvert<char16_t>(formula_);
  }

 private:
  const std::string formula_;
};

class FormulaTimedDataService : public TimedDataService {
 public:
  virtual std::shared_ptr<TimedData> GetNodeTimedData(
      const scada::NodeId& node_id,
      const scada::AggregateFilter& aggregation) override {
    return nullptr;
  }
  virtual std::shared_ptr<TimedData> GetFormulaTimedData(
      std::string_view formula,
      const scada::AggregateFilter& aggregation) override {
    return std::make_shared<FormulaTimedData>(std::string{formula});
  }
};

// A Table expression row selects a node-less spec that still carries a live
// value; the inspector shows it (regression: it used to clear on any null
// node id).
TEST_F(InspectorPanelTest, FormulaRowSelectionFillsTheInspector) {
  FormulaTimedDataService service;
  SelectionModel selection{{service}};
  TimedDataSpec spec{service, "{TIT.200}+{TIT.201}"};
  selection.SelectTimedData(spec);

  InspectorPanel panel{InspectorPanelContext{}};
  panel.ShowSelection(selection);

  auto* stack =
      panel.findChild<QStackedWidget*>(QStringLiteral("inspectorStack"));
  ASSERT_NE(stack, nullptr);
  EXPECT_EQ(stack->currentIndex(), 1);

  auto* subtitle =
      panel.findChild<QLabel*>(QStringLiteral("inspectorSubtitle"));
  ASSERT_NE(subtitle, nullptr);
  EXPECT_EQ(subtitle->text(), QStringLiteral("{TIT.200}+{TIT.201}"));
}

// A spec with neither node nor formula (a folder/object selection) still
// clears to the empty state.
TEST_F(InspectorPanelTest, DataLessSelectionShowsTheEmptyState) {
  FormulaTimedDataService service;
  SelectionModel selection{{service}};
  selection.SelectTimedData(TimedDataSpec{});

  InspectorPanel panel{InspectorPanelContext{}};
  panel.ShowSelection(selection);

  auto* stack =
      panel.findChild<QStackedWidget*>(QStringLiteral("inspectorStack"));
  ASSERT_NE(stack, nullptr);
  EXPECT_EQ(stack->currentIndex(), 0);
}

}  // namespace
