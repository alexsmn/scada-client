#pragma once

#include "aui/test/app_environment.h"
#include "controller/test/controller_environment.h"

#include <gmock/gmock.h>

class ControllerTest : public testing::TestWithParam<unsigned /*command_id*/> {
 protected:
  AppEnvironment app_env_;
  ControllerEnvironment controller_env_;
};

TEST_P(ControllerTest, Init) {
  auto command_id = GetParam();

  // TODO: Register the controller.
  auto controller_factory =
      controller_env_.controller_registry_.GetControllerFactory(command_id);

  ASSERT_THAT(controller_factory, NotNull());

  auto controller = controller_factory(controller_env_.MakeControllerContext());

  ASSERT_THAT(controller, NotNull());

  auto view = controller->Init(/*window_def=*/{});

  ASSERT_THAT(view, NotNull());
}