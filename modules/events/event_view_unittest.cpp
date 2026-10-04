#include "events/event_view.h"

#include "aui/test/app_environment.h"
#include "controller/command_handler.h"
#include "controller/test/controller_environment.h"
#include "events/local_events.h"
#include "profile/window_definition.h"
#include "resources/common_resources.h"

#include <gmock/gmock.h>

using namespace testing;

namespace {

// The events panel (the docked Current-mode view) keeps its severity
// threshold on the event fetcher, not on the table model. The provider below
// remembers what it is set to, the way the real fetcher does, so the tests
// can see where a menu command actually sends the threshold.
class EventViewPanelSeverityTest : public Test {
 protected:
  void SetUp() override {
    ON_CALL(env_.node_event_provider_, severity_min()).WillByDefault([this] {
      return fetch_severity_min_;
    });
    ON_CALL(env_.node_event_provider_, SetSeverityMin(_))
        .WillByDefault([this](scada::EventSeverity severity) {
          fetch_severity_min_ = severity;
        });

    view_ = std::make_unique<EventView>(env_.MakeControllerContext(),
                                        local_events_, /*is_panel=*/true);
    ui_view_ = view_->Init(WindowDefinition{});
    ASSERT_THAT(ui_view_, NotNull());
  }

  bool IsChecked(unsigned command_id) {
    CommandHandler* handler = view_->GetCommandHandler(command_id);
    EXPECT_THAT(handler, NotNull());
    return handler && handler->IsCommandChecked(command_id);
  }

  void Execute(unsigned command_id) {
    CommandHandler* handler = view_->GetCommandHandler(command_id);
    ASSERT_THAT(handler, NotNull());
    handler->ExecuteCommand(command_id);
  }

  AppEnvironment app_env_;
  ControllerEnvironment env_;
  LocalEvents local_events_;

  scada::EventSeverity fetch_severity_min_ = scada::kSeverityMin;

  std::unique_ptr<EventView> view_;
  std::unique_ptr<UiView> ui_view_;
};

}  // namespace

// Backlog 847: a custom threshold (as restored from the profile's
// `severityMin`, or chosen with Custom...) must read as Custom, not All.
TEST_F(EventViewPanelSeverityTest, CustomFetchThresholdIsShownAsCustom) {
  fetch_severity_min_ = static_cast<scada::EventSeverity>(500);

  EXPECT_TRUE(IsChecked(ID_SEVERITY_CUSTOM));
  EXPECT_FALSE(IsChecked(ID_SEVERITY_ALL));
}

// Backlog 847: All must lower the fetch threshold itself. It used to reset only
// the table model's own filter, so events below the old threshold were never
// fetched while the menu claimed All.
TEST_F(EventViewPanelSeverityTest, AllLowersTheFetchThreshold) {
  fetch_severity_min_ = static_cast<scada::EventSeverity>(500);

  Execute(ID_SEVERITY_ALL);

  EXPECT_EQ(fetch_severity_min_, scada::kSeverityMin);
  EXPECT_TRUE(IsChecked(ID_SEVERITY_ALL));
  EXPECT_FALSE(IsChecked(ID_SEVERITY_CUSTOM));
}

TEST_F(EventViewPanelSeverityTest, DefaultThresholdIsAll) {
  EXPECT_TRUE(IsChecked(ID_SEVERITY_ALL));
  EXPECT_FALSE(IsChecked(ID_SEVERITY_CUSTOM));
}
