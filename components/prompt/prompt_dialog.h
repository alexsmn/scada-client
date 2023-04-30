#pragma once

#include "base/promise.h"

#include <string>

class DialogService;

promise<std::u16string> RunPromptDialog(
    DialogService& dialog_service,
    const std::u16string& prompt,
    const std::u16string& title,
    const std::u16string& initial_value = {});
