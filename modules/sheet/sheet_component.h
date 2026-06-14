#pragma once

#include "controller/window_info.h"

class UiCommandRegistry;

extern const WindowInfo kSheetWindowInfo;

void RegisterSheetCommandActions(UiCommandRegistry& ui_command_registry);
