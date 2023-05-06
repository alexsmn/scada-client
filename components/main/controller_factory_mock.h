#pragma once

#include <gmock/gmock.h>

#include "components/main/controller_factory.h"

using MockControllerFactory = testing::MockFunction<std::unique_ptr<Controller>(
    unsigned command_id,
    ControllerDelegate& delegate,
    DialogService& dialog_service)>;
