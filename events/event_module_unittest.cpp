#include "events/event_module.h"

#include "aui/dialog_service_mock.h"
#include "aui/test/app_environment.h"
#include "base/logger.h"
#include "controller/selection_model.h"
#include "controller/test/controller_environment.h"
#include "controller/window_info.h"
#include "core/selection_command_context.h"
#include "main_window/main_window_mock.h"
#include "main_window/opened_view_interface.h"
#include "resources/common_resources.h"

#include <gmock/gmock.h>

#include "base/debug_util.h"

using namespace testing;

class EventModuleTest : public Test {
 protected:
  AppEnvironment app_env_;
  ControllerEnvironment controller_env_;

  EventModule event_module_{EventModuleContext{
      .executor_ = controller_env_.executor_,
      .logger_ = NullLogger::GetInstance(),
      .profile_ = controller_env_.profile_,
      .services_ = controller_env_.services(),
      .controller_registry_ = controller_env_.controller_registry_,
      .selection_commands_ = controller_env_.selection_commands_}};
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

  promise<WindowDefinition> GetOpenWindowDefinition(
      const WindowInfo* /*window_info*/) const override {
    return make_resolved_promise(open_def_);
  }

 private:
  const WindowDefinition open_def_;
  const WindowInfo info_;
};

}  // namespace

// Regression coverage for the `AddOpenCommand` coroutine migration:
// dispatching the command must (a) await the `GetOpenWindowDefinition`
// promise, (b) inject the configured `mode` item, and (c) call
// `MainWindowInterface::OpenView` with the resulting definition. The
// pre-migration `.then(...)` chain provided the same end-to-end behavior.
TEST_F(EventModuleTest, OpenEventsCommandRoutesToMainWindowOpenView) {
  WindowDefinition base_def{"EventJournal"};
  base_def.set_title(u"Events Journal");

  FakeOpenedView opened_view{base_def, WindowInfo{.name = "EventJournal"}};
  StrictMock<MockMainWindow> main_window;
  StrictMock<MockDialogService> dialog_service;
  SelectionModel selection{{controller_env_.timed_data_service_}};

  WindowDefinition opened;
  EXPECT_CALL(main_window, OpenView(_, _))
      .WillOnce(Invoke(
          [&opened](const WindowDefinition& def, bool /*activate*/)
              -> promise<OpenedViewInterface*> {
            opened = def;
            return make_resolved_promise<OpenedViewInterface*>(nullptr);
          }));

  const auto* command =
      controller_env_.selection_commands_.FindCommand(ID_OPEN_EVENTS);
  ASSERT_THAT(command, NotNull());
  ASSERT_TRUE(command->execute_handler);

  command->execute_handler(SelectionCommandContext{
      .selection = selection,
      .dialog_service = dialog_service,
      .main_window = main_window,
      .opened_view = opened_view});

  // Drain the executor so the spawned coroutine runs to completion. Two
  // polls are enough: one for the initial co_spawn dispatch, one for the
  // post-await resumption. Pump a few extras for safety against
  // implementation-detail churn in the helpers.
  for (int i = 0; i < 4; ++i) {
    controller_env_.executor_->Poll();
  }

  EXPECT_EQ(opened.type, base_def.type);
  const auto* mode_item = opened.FindItem("mode");
  ASSERT_THAT(mode_item, NotNull());
  EXPECT_TRUE(mode_item->attributes.is_string());
  EXPECT_EQ(mode_item->attributes.as_string(), "Current");
}
