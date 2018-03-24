#pragma once

#include "base/strings/string16.h"

class DialogService;

bool RunPromptDialog(DialogService& dialog_service,
                     const base::string16& prompt,
                     const base::string16& title,
                     base::string16& value);
