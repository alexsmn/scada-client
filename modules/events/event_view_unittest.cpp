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
// threshold on its own table model, starting from and writing back to the
// profile (`Profile::event_panel_severity_min`). It must never reach the
// fetcher, which every other alarm surface reads (backlog 889) -- so the
// provider here fails the test if anything sets its threshold.
class EventViewPanelSeverityTest : public Test {
 protected:
  void SetUp() override {
    EXPECT_CALL(env_.node_event_provider_, SetSeverityMin(_)).Times(0);
  }

  void CreateView() {
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

  std::unique_ptr<EventView> view_;
  std::unique_ptr<UiView> ui_view_;
};

}  // namespace

// Backlog 847: a custom threshold (as restored from the profile's
// `severityMin`, or chosen with Custom...) must read as Custom, not All.
TEST_F(EventViewPanelSeverityTest, RestoredThresholdIsShownAsCustom) {
  env_.profile_.event_panel_severity_min = 500;
  CreateView();

  EXPECT_TRUE(IsChecked(ID_SEVERITY_CUSTOM));
  EXPECT_FALSE(IsChecked(ID_SEVERITY_ALL));
}

// Backlog 847: All must clear the threshold the panel saves, not only the
// filter on screen, or the next start restores the old one.
TEST_F(EventViewPanelSeverityTest, AllClearsTheSavedThreshold) {
  env_.profile_.event_panel_severity_min = 500;
  CreateView();

  Execute(ID_SEVERITY_ALL);

  EXPECT_EQ(env_.profile_.event_panel_severity_min,
            static_cast<unsigned>(scada::kSeverityMin));
  EXPECT_TRUE(IsChecked(ID_SEVERITY_ALL));
  EXPECT_FALSE(IsChecked(ID_SEVERITY_CUSTOM));
}

TEST_F(EventViewPanelSeverityTest, DefaultThresholdIsAll) {
  CreateView();
  EXPECT_TRUE(IsChecked(ID_SEVERITY_ALL));
  EXPECT_FALSE(IsChecked(ID_SEVERITY_CUSTOM));
}
