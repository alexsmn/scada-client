#pragma once

#include <string>

class DialogService;

bool RunPromptDialog(DialogService& dialog_service,
                     const std::wstring& prompt,
                     const std::wstring& title,
                     std::wstring& value);
