#pragma once

#include <string>

class DialogService;

bool RunPromptDialog(DialogService& dialog_service,
                     const std::u16string& prompt,
                     const std::u16string& title,
                     std::u16string& value);
