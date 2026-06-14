#pragma once

#include "controller/window_info.h"

class UiCommandRegistry;

extern const WindowInfo kWatchWindowInfo;

void RegisterWatchCommandActions(UiCommandRegistry& ui_command_registry);
