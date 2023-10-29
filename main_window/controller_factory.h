#pragma once

#include "controller/controller.h"

#include <functional>
#include <memory>

class ControllerDelegate;
class DialogService;

using ControllerFactory =
    std::function<std::unique_ptr<Controller>(unsigned command_id,
                                              ControllerDelegate& delegate,
                                              DialogService& dialog_service)>;
