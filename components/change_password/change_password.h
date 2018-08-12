#pragma once

#include "core/localized_text.h"

struct ChangePasswordContext;

void ChangePassword(const ChangePasswordContext& context,
                    const scada::LocalizedText& current_password,
                    const scada::LocalizedText& new_password);
