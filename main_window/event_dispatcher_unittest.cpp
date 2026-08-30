#include "main_window/event_dispatcher.h"

#include "base/test/test_executor.h"
#include "controller/action_manager.h"
#include "events/local_events.h"
#include "events/node_event_provider_mock.h"
#include "profile/profile.h"
#include "services/speech_service_mock.h"

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

// The spoken announcement behind «Speech». It is a separate option from the
// tone and takes its own edge, which is the defect these tests pin: the option
// existed, persisted and was togglable, but nothing anywhere read
// `Profile::speech_enabled` and `SpeechService::Speak` had no caller in the
// client at all, so ticking the box announced nothing.
class EventDispatcherSpeechTest : public Test {
 protected:
  void SetUp() override {
    profile_.speech_enabled = true;
    ON_CALL(speech_service_, is_ok()).WillByDefault(Return(true));
  }

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

  NiceMock<MockSpeechService> speech_service_;

  EventDispatcher dispatcher_{
      EventDispatcherContext{.executor_ = executor_,
                             .node_event_provider_ = node_event_provider_,
                             .local_events_ = local_events_,
                             .profile_ = profile_,
                             .events_handler_ = [](bool) {},
                             .action_manager_ = action_manager_,
                             .speech_service_ = &speech_service_,
                             .event_debounce_ = {}}};
};

TEST_F(EventDispatcherSpeechTest, AnnouncesWhenAnAlarmArrives) {
  EXPECT_CALL(speech_service_, Speak(_));

  RaiseAlarm();
  Dispatch();
}

// Independent of the tone, which is the whole point of the separate latch: the
// two are separate options and «Speech» must work with the sound switched off.
TEST_F(EventDispatcherSpeechTest, AnnouncesEvenWhenTheAlarmToneIsOff) {
  profile_.event_play_sound = false;

  EXPECT_CALL(speech_service_, Speak(_));

  RaiseAlarm();
  Dispatch();
}

TEST_F(EventDispatcherSpeechTest, StaysSilentWhenTheOptionIsOff) {
  profile_.speech_enabled = false;

  EXPECT_CALL(speech_service_, Speak(_)).Times(0);

  RaiseAlarm();
  Dispatch();
}

// A build with no voice — every non-Windows build — must not be told it
// announced anything.
TEST_F(EventDispatcherSpeechTest, StaysSilentWithoutAVoice) {
  ON_CALL(speech_service_, is_ok()).WillByDefault(Return(false));

  EXPECT_CALL(speech_service_, Speak(_)).Times(0);

  RaiseAlarm();
  Dispatch();
}

// Speaks as the alarm arrives and not again while it stands, and says nothing
// at all when the last event is acknowledged — there is nothing to announce on
// the falling edge, and repeating it would talk over the operator.
TEST_F(EventDispatcherSpeechTest, AnnouncesOnceOnTheRisingEdgeOnly) {
  EXPECT_CALL(speech_service_, Speak(_)).Times(1);

  RaiseAlarm();
  Dispatch();

  RaiseAlarm();
  Dispatch();

  AcknowledgeAll();
  Dispatch();
}

}  // namespace
