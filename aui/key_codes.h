#pragma once

#if defined(UI_QT)
#include <qnamespace.h>
#elif defined(UI_WT)
#include <Wt/WGlobal.h>
#endif

namespace aui {

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

#elif defined(UI_WT)
/*enum class KeyCode : int {
  Escape = Wt::Key::Escape,
  Enter = Wt::Key::Enter,
  Delete = Wt::Key::Delete,
  Up = Wt::Key::Up,
  Down = Wt::Key::Down,
};*/
using KeyCode = Wt::Key;

// Wt flags are too restrictive. &, |= are not supported.
using KeyModifier = unsigned;

constexpr KeyModifier ShiftModifier =
    static_cast<unsigned>(Wt::KeyboardModifier::Shift);
constexpr KeyModifier ControlModifier =
    static_cast<unsigned>(Wt::KeyboardModifier::Control);
constexpr KeyModifier AltModifier =
    static_cast<unsigned>(Wt::KeyboardModifier::Alt);

using KeyModifiers = unsigned;
#endif

}  // namespace aui

#if defined(UI_QT)
#include "aui/qt/key_codes.h"
#endif
