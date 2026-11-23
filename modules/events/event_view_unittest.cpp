#include "events/event_view.h"

#include "aui/dialog_service.h"
#include "aui/qt/table.h"
#include "aui/test/app_environment.h"
#include "base/check.h"
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

namespace {

// Backlog 1119 (decided 2026-11-23): acknowledging in the journal is
// read-then-act. A double-click opens the row in the Inspector instead of
// acknowledging it, and «Квитировать все» asks a default-No question once it
// would acknowledge more than one event. The Client's own local events are
// used because they are always acknowledgeable, whatever the session's rights.
class EventViewAcknowledgeTest : public Test {
 protected:
  void CreateView() {
    view_ = std::make_unique<EventView>(env_.MakeControllerContext(),
                                        local_events_, /*is_panel=*/true);
    ui_view_ = view_->Init(WindowDefinition{});
    ASSERT_THAT(ui_view_, NotNull());
  }

  scada::aui::Table& table() {
    // The docked panel's view is the table itself (no filter bar or footer).
    auto* table = dynamic_cast<scada::aui::Table*>(ui_view_.get());
    scada::base::Check(table != nullptr, "the panel's view is not a table");
    return *table;
  }

  void Report(std::u16string_view message) {
    local_events_.ReportEvent(LocalEvents::SEV_ERROR,
                              scada::LocalizedText{std::u16string{message}});
  }

  void AcknowledgeAll() {
    CommandHandler* handler = view_->GetCommandHandler(ID_ACKNOWLEDGE_ALL);
    ASSERT_THAT(handler, NotNull());
    ASSERT_TRUE(handler->IsCommandEnabled(ID_ACKNOWLEDGE_ALL));
    handler->ExecuteCommand(ID_ACKNOWLEDGE_ALL);
    env_.executor_.Poll();
  }

  AppEnvironment app_env_;
  ControllerEnvironment env_;
  LocalEvents local_events_;

  std::unique_ptr<EventView> view_;
  std::unique_ptr<UiView> ui_view_;
};

Awaitable<MessageBoxResult> Answer(MessageBoxResult result) {
  co_return result;
}

}  // namespace

TEST_F(EventViewAcknowledgeTest,
       DoubleClickRevealsTheInspectorAndKeepsTheEvent) {
  Report(u"breaker tripped");
  CreateView();
  ASSERT_EQ(table().model()->rowCount(), 1);
  table().SelectRow(0);

  EXPECT_CALL(env_.controller_delegate_, RevealInspector()).Times(1);
  Q_EMIT table().doubleClicked(table().model()->index(0, 1));

  EXPECT_EQ(local_events_.events().size(), 1u);
}

TEST_F(EventViewAcknowledgeTest, AcknowledgeAllOfOneEventDoesNotAsk) {
  Report(u"breaker tripped");
  CreateView();

  EXPECT_CALL(env_.dialog_service_, RunMessageBox(_, _, _)).Times(0);
  AcknowledgeAll();

  EXPECT_TRUE(local_events_.events().empty());
}

TEST_F(EventViewAcknowledgeTest, AcknowledgeAllOfSeveralAsksAndNoKeepsThem) {
  Report(u"breaker tripped");
  Report(u"feeder lost");
  CreateView();

  // Default No: the platform's Return answers the question with No.
  std::u16string asked;
  EXPECT_CALL(env_.dialog_service_,
              RunMessageBox(_, _, MessageBoxMode::QuestionYesNoDefaultNo))
      .WillOnce([&asked](std::u16string_view message, std::u16string_view,
                         MessageBoxMode) {
        asked = message;
        return Answer(MessageBoxResult::No);
      });
  AcknowledgeAll();

  // The question names how many events Yes would acknowledge.
  EXPECT_NE(asked.find(u"2"), std::u16string::npos);

  EXPECT_EQ(local_events_.events().size(), 2u);
}

// Yes acknowledges the events the question counted, not one that arrived
// while it was open.
TEST_F(EventViewAcknowledgeTest,
       AcknowledgeAllYesAcknowledgesOnlyWhatItCounted) {
  Report(u"breaker tripped");
  Report(u"feeder lost");
  CreateView();

  EXPECT_CALL(env_.dialog_service_,
              RunMessageBox(_, _, MessageBoxMode::QuestionYesNoDefaultNo))
      .WillOnce(
          [this](std::u16string_view, std::u16string_view, MessageBoxMode) {
            Report(u"arrived while asking");
            return Answer(MessageBoxResult::Yes);
          });
  AcknowledgeAll();

  ASSERT_EQ(local_events_.events().size(), 1u);
  EXPECT_EQ(local_events_.events().front()->message,
            scada::LocalizedText{u"arrived while asking"});
}
