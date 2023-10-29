#pragma once

#include "main_window/controller_factory.h"

#include <gmock/gmock.h>

using MockControllerFactory = testing::MockFunction<std::unique_ptr<Controller>(
    unsigned command_id,
    ControllerDelegate& delegate,
    DialogService& dialog_service)>;
