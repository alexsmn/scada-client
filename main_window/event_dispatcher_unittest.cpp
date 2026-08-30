#include "main_window/event_dispatcher.h"

#include "base/test/test_executor.h"
#include "controller/action_manager.h"
#include "events/local_events.h"
#include "events/node_event_provider_mock.h"
#include "profile/profile.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace testing;

namespace {

// The audible annunciator behind «Sound Alarm on Event». The tone itself is the
// platform's — a looping system alias on Windows, `QApplication::beep` on every
// other platform — so what is asserted here is the request, which is as close
// to the speaker as a test can get. It is enough to catch the defect these
// tests were written for: the emission used to sit inside `#if defined(_WIN32)`
// along with the edge detection, so on macOS the option was live, persisted,
// and synchronised to the web client while doing nothing at all.
class EventDispatcherAlarmSoundTest : public Test {
 protected:
  void SetUp() override { profile_.event_play_sound = true; }

  // The dispatcher coalesces arriving events onto a posted task. The fixture
  // sets the debounce to zero so that task is posted rather than timed, which
  // keeps the whole test deterministic: `PostDelayedTask` only reaches for a
  // real `steady_timer` on a non-zero delay, and no timer is one `TestExecutor`
  // could drive.
  void Dispatch() { executor_.Poll(); }

  void RaiseAlarm() {
    local_events_.ReportEvent(LocalEvents::SEV_ERROR, u"comms lost");
  }

  void AcknowledgeAll() { local_events_.AcknowledgeAll(); }

  TestExecutor executor_;
  NiceMock<MockNodeEventProvider> node_event_provider_;
  LocalEvents local_events_;
  Profile profile_;
  ActionManager action_manager_;

  StrictMock<MockFunction<void(bool playing)>> alarm_sound_;

  EventDispatcher dispatcher_{EventDispatcherContext{
      .executor_ = executor_,
      .node_event_provider_ = node_event_provider_,
      .local_events_ = local_events_,
      .profile_ = profile_,
      .events_handler_ = [](bool) {},
      .action_manager_ = action_manager_,
      .alarm_sound_handler_ = alarm_sound_.AsStdFunction(),
      .event_debounce_ = {}}};
};

TEST_F(EventDispatcherAlarmSoundTest, AnnunciatesWhenAnAlarmArrives) {
  EXPECT_CALL(alarm_sound_, Call(true));

  RaiseAlarm();
  Dispatch();

  EXPECT_TRUE(dispatcher_.playing_alarm_sound());
}

TEST_F(EventDispatcherAlarmSoundTest, StopsWhenTheLastAlarmIsAcknowledged) {
  EXPECT_CALL(alarm_sound_, Call(true));

  RaiseAlarm();
  Dispatch();
  ASSERT_TRUE(dispatcher_.playing_alarm_sound());

  EXPECT_CALL(alarm_sound_, Call(false));

  AcknowledgeAll();
  Dispatch();

  EXPECT_FALSE(dispatcher_.playing_alarm_sound());
}

// The handler is edge-triggered: a second alarm while one already stands is not
// a second annunciation. `StrictMock` is what enforces it — an extra `Call` is
// a failure rather than an ignored expectation.
TEST_F(EventDispatcherAlarmSoundTest, DoesNotReannunciateWhileAnAlarmStands) {
  EXPECT_CALL(alarm_sound_, Call(true));

  RaiseAlarm();
  Dispatch();

  RaiseAlarm();
  Dispatch();

  EXPECT_TRUE(dispatcher_.playing_alarm_sound());
}

// The option gates the tone, and it is the whole of what an operator ticking
// the box in Settings controls.
TEST_F(EventDispatcherAlarmSoundTest, StaysSilentWhenTheOptionIsOff) {
  profile_.event_play_sound = false;

  RaiseAlarm();
  Dispatch();

  EXPECT_FALSE(dispatcher_.playing_alarm_sound());
}

}  // namespace
