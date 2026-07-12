#pragma once

#include "aui/aui_ns_compat.h"

namespace scada::aui {

// For a separator we have the following types.
enum MenuSeparatorType {
  // Normal - top to bottom: Spacing, line, spacing
  NORMAL_SEPARATOR,

  // Upper - top to bottom: Line, spacing
  UPPER_SEPARATOR,

  // Lower - top to bottom: Spacing, line
  LOWER_SEPARATOR,

  // Spacing - top to bottom: Spacing only.
  SPACING_SEPARATOR
};

}  // namespace aui
