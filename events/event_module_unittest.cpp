#include "events/event_module.h"

#include "aui/test/app_environment.h"
#include "base/logger.h"
#include "controller/test/controller_environment.h"

#include <gmock/gmock.h>

#include "base/debug_util-inl.h"

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
