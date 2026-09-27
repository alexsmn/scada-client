#pragma once

#include "aui/models/color_role.h"

#include <QVariant>

namespace scada::aui {

// Resolves a model's `ColorRole` into the platform's own text colour, read
// from the application palette at the time of the call so a theme switch
// repaints correctly. This is the seam the toolkit-free models cannot cross
// for themselves. Returns an unset `QVariant` for `Default`, leaving the
// view's own colour alone.
QVariant ColorRoleForeground(ColorRole role);

// The background counterpart of `ColorRoleForeground`. Unset for the roles
// that recolour text only.
QVariant ColorRoleBackground(ColorRole role);

}  // namespace scada::aui
