#pragma once

#include "base/awaitable.h"

#include <string>

class DialogService;

Awaitable<std::u16string> RunPromptDialog(
    DialogService& dialog_service,
    const std::u16string& prompt,
    const std::u16string& title,
    const std::u16string& initial_value = {});
