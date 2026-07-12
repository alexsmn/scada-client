#include "main_window/activity_bar_qt.h"

#include "aui/test/app_environment.h"
#include "main_window/status_bar/status_bar_model_impl.h"

#include <QToolButton>

#include <gtest/gtest.h>

#include <string>
#include <vector>

namespace {

ActivityBar::Section MakeSection(std::string name, bool enabled) {
  ActivityBar::Section section;
  section.window_info_name = std::move(name);
  section.label = u"Section";
  section.enabled = enabled;
  return section;
}

// The rail's section buttons, in creation order (top group, then bottom group).
std::vector<QToolButton*> Buttons(const ActivityBar& bar) {
  const QList<QToolButton*> found = bar.findChildren<QToolButton*>();
  return {found.begin(), found.end()};
}

class ActivityBarTest : public ::testing::Test {
 protected:
  // Per-test QApplication (Qt requires one before any QWidget), destroyed
  // with the fixture. Never keep a static QApplication in a test binary: it
  // is destroyed during atexit teardown, where ~QGuiApplication crashes on
  // macOS after other Qt statics are already gone.
  AppEnvironment app_env_;
};

TEST_F(ActivityBarTest, ClickingAnEnabledSectionActivatesIt) {
  std::vector<std::string> activated;
  std::vector<ActivityBar::Section> sections;
  sections.push_back(MakeSection("alarms", /*enabled=*/true));

  ActivityBar bar{nullptr, std::move(sections),
                  [&](const std::string& name) { activated.push_back(name); }};

  std::vector<QToolButton*> buttons = Buttons(bar);
  ASSERT_EQ(buttons.size(), 1u);
  buttons[0]->click();

  EXPECT_EQ(activated, (std::vector<std::string>{"alarms"}));
}

TEST_F(ActivityBarTest, DisabledSectionDoesNotActivate) {
  std::vector<std::string> activated;
  std::vector<ActivityBar::Section> sections;
  sections.push_back(MakeSection("overview", /*enabled=*/false));

  ActivityBar bar{nullptr, std::move(sections),
                  [&](const std::string& name) { activated.push_back(name); }};

  std::vector<QToolButton*> buttons = Buttons(bar);
  ASSERT_EQ(buttons.size(), 1u);
  EXPECT_FALSE(buttons[0]->isEnabled());
  buttons[0]->click();  // disabled buttons swallow clicks

  EXPECT_TRUE(activated.empty());
}

TEST_F(ActivityBarTest, SetActiveSectionChecksTheMatchingButton) {
  std::vector<ActivityBar::Section> sections;
  sections.push_back(MakeSection("alarms", /*enabled=*/true));
  sections.push_back(MakeSection("tables", /*enabled=*/true));

  ActivityBar bar{nullptr, std::move(sections), [](const std::string&) {}};
  bar.SetActiveSection("tables");

  std::vector<QToolButton*> buttons = Buttons(bar);
  ASSERT_EQ(buttons.size(), 2u);
  EXPECT_FALSE(buttons[0]->isChecked());
  EXPECT_TRUE(buttons[1]->isChecked());
}

TEST_F(ActivityBarTest, SetAlarmCountLeavesTheAlarmsButtonUsable) {
  std::vector<ActivityBar::Section> sections;
  ActivityBar::Section alarms = MakeSection("alarms", /*enabled=*/true);
  alarms.is_alarms = true;
  sections.push_back(std::move(alarms));

  ActivityBar bar{nullptr, std::move(sections), [](const std::string&) {}};
  bar.SetAlarmCount(7);  // paints a badge
  bar.SetAlarmCount(0);  // clears it

  std::vector<QToolButton*> buttons = Buttons(bar);
  ASSERT_EQ(buttons.size(), 1u);
  EXPECT_FALSE(buttons[0]->icon().isNull());
}

// The status-bar model surfaces the unacknowledged-alarm count that the rail
// badge reads; it returns 0 with no provider and the provider's value once set.
TEST(StatusBarModelAlarmCountTest, ReportsProviderValue) {
  StatusBarModelImpl model;
  EXPECT_EQ(model.GetAlarmCount(), 0);

  int count = 3;
  model.SetAlarmCountProvider([&count] { return count; });
  EXPECT_EQ(model.GetAlarmCount(), 3);

  count = 12;
  EXPECT_EQ(model.GetAlarmCount(), 12);
}

}  // namespace
