#pragma once

#if defined(UI_QT)
#include <qnamespace.h>
#endif

namespace scada::aui {

#if defined(UI_QT)
enum class KeyCode : int {
  Escape = Qt::Key_Escape,
  Enter = Qt::Key_Enter,
  Delete = Qt::Key_Delete,
  Up = Qt::Key_Up,
  Down = Qt::Key_Down,
  F2 = Qt::Key_F2,
  C = Qt::Key_C,
  V = Qt::Key_V,
  Unknown = Qt::Key_unknown,
};

using KeyModifier = Qt::KeyboardModifier;

constexpr KeyModifier ShiftModifier = Qt::ShiftModifier;
constexpr KeyModifier ControlModifier = Qt::ControlModifier;
constexpr KeyModifier AltModifier = Qt::AltModifier;

using KeyModifiers = Qt::KeyboardModifiers;

#endif

}  // namespace aui

#if defined(UI_QT)
#include "aui/qt/key_codes.h"
#endif
