#pragma once

#include "aui/key_codes.h"

#include <QKeyCombination>
#include <QKeySequence>

namespace scada::aui {

// The Qt shortcut for an aui key plus modifiers. A QKeyCombination composes
// the two the way Qt defines it; the previous two hand-rolled copies *added*
// the enum values, which happened to work only because the bits do not
// overlap.
inline QKeySequence ToQKeySequence(KeyModifiers modifiers, KeyCode key_code) {
  return QKeySequence{
      QKeyCombination{modifiers, static_cast<Qt::Key>(key_code)}};
}

}  // namespace scada::aui
