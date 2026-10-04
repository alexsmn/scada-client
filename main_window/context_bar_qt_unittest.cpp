#include "main_window/context_bar_qt.h"

#include "aui/test/app_environment.h"
#include "main_window/command_field_qt.h"

#include <QApplication>
#include <QLabel>
#include <QMouseEvent>

#include <gtest/gtest.h>

#include <memory>
#include <vector>

namespace {

class ContextBarTest : public ::testing::Test {
 protected:
  ContextBar& MakeBar() {
    bar_ = std::make_unique<ContextBar>(
        [this] { return counts_; },
        [this](const QString& text) { palette_opens_.push_back(text); },
        nullptr);
    return *bar_;
  }

  // Per-test QApplication; see ActivityBarTest for why it is never static.
  AppEnvironment app_env_;
  events::SeverityTileCounts counts_;
  std::vector<QString> palette_opens_;
  std::unique_ptr<ContextBar> bar_;
};

// The screenshot generator's shell assertions find the bar and its three
// slots by these names, so they are part of the bar's contract.
TEST_F(ContextBarTest, CarriesTheObjectNamesTheShellAssertionsUse) {
  ContextBar& bar = MakeBar();

  EXPECT_EQ(bar.objectName(), QStringLiteral("ContextBar"));
  EXPECT_NE(bar.findChild<QWidget*>(QStringLiteral("contextBarLeftSlot")),
            nullptr);
  EXPECT_NE(bar.findChild<QWidget*>(QStringLiteral("contextBarCentreSlot")),
            nullptr);
  EXPECT_NE(bar.findChild<QWidget*>(QStringLiteral("contextBarRightSlot")),
            nullptr);
}

TEST_F(ContextBarTest, SetBreadcrumbShowsTheSegments) {
  ContextBar& bar = MakeBar();

  const Breadcrumb::Segment segments[] = {
      {.label = QStringLiteral("Page"), .strong = true},
      {.label = QStringLiteral("Table")},
      {.label = QStringLiteral("Pump 1"), .strong = true},
  };
  bar.SetBreadcrumb(segments);

  auto* breadcrumb = bar.findChild<Breadcrumb*>();
  ASSERT_NE(breadcrumb, nullptr);
  EXPECT_TRUE(breadcrumb->Text().contains(QStringLiteral("Pump 1")));
}

// The bar does not watch the alarm model itself: it re-reads the counts when
// the host says they moved.
TEST_F(ContextBarTest, RefreshAlarmStateRereadsTheCounts) {
  ContextBar& bar = MakeBar();
  // AlarmStateCluster carries no Q_OBJECT, so findChild cannot search for it
  // by type; its slot name is part of the bar's contract anyway.
  auto* alarms = dynamic_cast<AlarmStateCluster*>(
      bar.findChild<QWidget*>(QStringLiteral("contextBarRightSlot")));
  ASSERT_NE(alarms, nullptr);
  EXPECT_TRUE(alarms->annunciator()->isHidden());

  counts_ = {.critical = 1, .unacknowledged = 1};
  bar.RefreshAlarmState();

  EXPECT_FALSE(alarms->annunciator()->isHidden());
}

TEST_F(ContextBarTest, ClickingTheFieldOpensThePalette) {
  ContextBar& bar = MakeBar();
  auto* field = bar.findChild<CommandField*>();
  ASSERT_NE(field, nullptr);

  QMouseEvent release{QEvent::MouseButtonRelease,
                      QPointF{1, 1},
                      QPointF{1, 1},
                      QPointF{1, 1},
                      Qt::LeftButton,
                      Qt::NoButton,
                      Qt::NoModifier};
  QApplication::sendEvent(field, &release);

  ASSERT_EQ(palette_opens_.size(), 1u);
  EXPECT_TRUE(palette_opens_.front().isEmpty());
}

}  // namespace
