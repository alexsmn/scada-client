#include "events/event_module.h"

#include "aui/dialog_service_mock.h"
#include "aui/test/app_environment.h"
#include "base/awaitable.h"
#include "base/boost_log.h"
#include "controller/command_ui_registry.h"
#include "controller/selection_model.h"
#include "controller/test/controller_environment.h"
#include "controller/window_info.h"
#include "core/selection_command_context.h"
#include "events/event_fetcher.h"
#include "main_window/main_window_mock.h"
#include "main_window/opened_view/opened_view_interface.h"
#include "resources/common_resources.h"

#include <gmock/gmock.h>

#include "base/debug_util.h"

using namespace testing;

class EventModuleTest : public Test {
 protected:
  AppEnvironment app_env_;
  ControllerEnvironment controller_env_;
  UiCommandRegistry ui_command_registry_;

  EventModule event_module_{EventModuleContext{
      .executor_ = controller_env_.executor_,
      .logger_ = std::make_shared<BoostLogger>(LOG_NAME("Test")),
      .profile_ = controller_env_.profile_,
      .services_ = controller_env_.services(),
      .controller_registry_ = controller_env_.controller_registry_,
      .global_commands_ = controller_env_.global_commands_,
      .selection_commands_ = controller_env_.selection_commands_,
      .ui_command_registry_ = ui_command_registry_}};
};

TEST_F(EventModuleTest, CreateControllers) {
  controller_env_.TestController(ID_EVENT_VIEW);
  controller_env_.TestController(ID_EVENT_JOURNAL_VIEW);
}

namespace {

// Minimal `OpenedViewInterface` fake for command-handler tests. The
// `event_module` `AddOpenCommand` migration only needs a way to feed a
// pre-resolved `WindowDefinition` into the coroutine; everything else is
// inert.
class FakeOpenedView : public OpenedViewInterface {
 public:
  FakeOpenedView(WindowDefinition open_def, WindowInfo info)
      : open_def_{std::move(open_def)}, info_{std::move(info)} {}

  const WindowInfo& GetWindowInfo() const override { return info_; }
  std::u16string GetWindowTitle() const override { return {}; }
  void SetWindowTitle(std::u16string_view) override {}
  WindowDefinition Save() override { return {}; }
  ContentsModel* GetContents() override { return nullptr; }
  void Select(const scada::NodeId&) override {}

  Awaitable<WindowDefinition> GetOpenWindowDefinition(
      const WindowInfo* /*window_info*/) const override {
    ++open_definition_await_count;
    co_return open_def_;
  }

  mutable int open_definition_await_count = 0;

 private:
  const WindowDefinition open_def_;
  const WindowInfo info_;
};

}  // namespace

// Regression coverage for the `AddOpenCommand` coroutine path: dispatching
// the command must await `GetOpenWindowDefinition`, inject the configured
// `mode` item, and call `MainWindowInterface::OpenView` with the result.
TEST_F(EventModuleTest, OpenEventsCommandRoutesToMainWindowOpenView) {
  WindowDefinition base_def{"EventJournal"};
  base_def.set_title(u"Events Journal");

  FakeOpenedView opened_view{base_def, WindowInfo{.name = "EventJournal"}};
  StrictMock<MockMainWindow> main_window;
  StrictMock<MockDialogService> dialog_service;
  SelectionModel selection{{controller_env_.timed_data_service_}};

  WindowDefinition opened;
  EXPECT_CALL(main_window, OpenView(_, _))
      .WillOnce(Invoke([&opened](const WindowDefinition& def, bool /*activate*/)
                           -> Awaitable<OpenedViewInterface*> {
        opened = def;
        co_return nullptr;
      }));

  const auto* command =
      controller_env_.selection_commands_.FindCommand(ID_OPEN_EVENTS);
  ASSERT_THAT(command, NotNull());
  ASSERT_TRUE(command->execute_handler);

  command->execute_handler(
      SelectionCommandContext{.selection = selection,
                              .dialog_service = dialog_service,
                              .main_window = main_window,
                              .opened_view = opened_view});

  EXPECT_EQ(opened_view.open_definition_await_count, 0);
  EXPECT_TRUE(opened.type.empty());

  // Drain the executor so the spawned coroutine runs to completion. Two
  // polls are enough: one for the initial co_spawn dispatch, one for the
  // post-await resumption. Pump a few extras for safety against
  // implementation-detail churn in the helpers.
  for (int i = 0; i < 4; ++i) {
    controller_env_.executor_.Poll();
  }

  EXPECT_EQ(opened_view.open_definition_await_count, 1);
  EXPECT_EQ(opened.type, base_def.type);
  const auto* mode_item = opened.FindItem("mode");
  ASSERT_THAT(mode_item, NotNull());
  EXPECT_TRUE(mode_item->attributes.is_string());
  EXPECT_EQ(mode_item->attributes.as_string(), "Current");
}

// The severity filter's 0-100 -> 1-1000 migration (ADR 0005 phase 1). These
// build their own module because the threshold is read once, in the
// constructor, so the profile has to carry its value before that runs.
class EventModuleSeverityMigrationTest : public Test {
 protected:
  // Seeds the profile, then reports the threshold the module ends up with.
  // Each call gets its own environment: EventModule registers its controller
  // factories on construction, and registering the same WindowInfo twice is
  // a hard error, so the environments cannot be shared across cases.
  unsigned SeverityMinFor(boost::json::object profile_data) {
    ControllerEnvironment controller_env;
    UiCommandRegistry ui_command_registry;
    controller_env.profile_.data() = std::move(profile_data);
    EventModule module{EventModuleContext{
        .executor_ = controller_env.executor_,
        .logger_ = std::make_shared<BoostLogger>(LOG_NAME("Test")),
        .profile_ = controller_env.profile_,
        .services_ = controller_env.services(),
        .controller_registry_ = controller_env.controller_registry_,
        .global_commands_ = controller_env.global_commands_,
        .selection_commands_ = controller_env.selection_commands_,
        .ui_command_registry_ = ui_command_registry}};
    return static_cast<unsigned>(module.event_fetcher().severity_min());
  }

  AppEnvironment app_env_;
};

// The regression: a profile that never stored a threshold must keep showing
// everything. Rescaling the default turned it into 10 and silently hid the
// severity-1 device-watch stream.
TEST_F(EventModuleSeverityMigrationTest, AbsentThresholdIsNotRescaled) {
  EXPECT_EQ(SeverityMinFor({}),
            static_cast<unsigned>(scada::kSeverityMin));
  // A profile carrying unrelated keys is still one that stored no threshold.
  EXPECT_EQ(SeverityMinFor({{"severityScale", 100}}),
            static_cast<unsigned>(scada::kSeverityMin));
}

// A threshold the user did store on the old scale still migrates, which is
// what the rescale exists for.
TEST_F(EventModuleSeverityMigrationTest, StoredOldScaleThresholdIsRescaled) {
  EXPECT_EQ(SeverityMinFor({{"severityMin", 50}}), 500u);
  EXPECT_EQ(SeverityMinFor({{"severityMin", 1}}), 10u);
}

// Once the marker is present the value is already on the 1-1000 scale and
// must pass through untouched, however many times the profile is reloaded.
TEST_F(EventModuleSeverityMigrationTest, NewScaleThresholdPassesThrough) {
  EXPECT_EQ(SeverityMinFor({{"severityMin", 500}, {"severityScale", 1000}}),
            500u);
  EXPECT_EQ(SeverityMinFor({{"severityMin", 1}, {"severityScale", 1000}}), 1u);
}

// Values from a corrupt or hand-edited profile must not escape the scale or
// overflow the multiply.
TEST_F(EventModuleSeverityMigrationTest, OutOfRangeThresholdIsClamped) {
  EXPECT_EQ(SeverityMinFor({{"severityMin", 0}}),
            static_cast<unsigned>(scada::kSeverityMin));
  EXPECT_EQ(SeverityMinFor({{"severityMin", -5}}),
            static_cast<unsigned>(scada::kSeverityMin));
  EXPECT_EQ(SeverityMinFor({{"severityMin", 999999}}),
            static_cast<unsigned>(scada::kSeverityMax));
}
