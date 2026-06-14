#pragma once

#include "controller/window_info.h"

class UiCommandRegistry;

extern const WindowInfo kTimedDataWindowInfo;

void RegisterTimedDataCommandActions(UiCommandRegistry& ui_command_registry);
