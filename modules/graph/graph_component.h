#pragma once

#include "controller/window_info.h"

class UiCommandRegistry;

extern const WindowInfo kGraphWindowInfo;

void RegisterGraphCommandActions(UiCommandRegistry& ui_command_registry);
