#pragma once

#include "controller/window_info.h"

class UiCommandRegistry;

extern const WindowInfo kTableWindowInfo;

void RegisterTableCommandActions(UiCommandRegistry& ui_command_registry);
