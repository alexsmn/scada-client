#pragma once

#if defined(UI_QT)
#include <qnamespace.h>
#elif defined(UI_VIEWS)
#include "ui/views/view.h"
#elif defined(UI_WT)
#pragma warning(push)
#pragma warning(disable : 4251)
#include <Wt/WGlobal.h>
#pragma warning(pop)
#endif

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

enum class KeyModifier : int {
  Shift = Qt::ShiftModifier,
  Control = Qt::ControlModifier,
  Alt = Qt::AltModifier
};

#elif defined(UI_VIEWS)
enum class KeyCode : unsigned {
  Escape = ui::KeyboardCode::VKEY_ESCAPE,
  Enter = ui::KeyboardCode::VKEY_RETURN,
  Delete = ui::KeyboardCode::VKEY_DELETE,
  Up = ui::KeyboardCode::VKEY_UP,
  Down = ui::KeyboardCode::VKEY_DOWN,
  Shift = ui::KeyboardCode::VKEY_SHIFT,
  Control = ui::KeyboardCode::VKEY_CONTROL,
  Alt = ui::KeyboardCode::VKEY_MENU,
  F2 = ui::KeyboardCode::VKEY_F2,
  C = ui::KeyboardCode::VKEY_C,
  V = ui::KeyboardCode::VKEY_V,
  Unknown = ui::KeyboardCode::VKEY_UNKNOWN,
};

enum class KeyModifier : int {
  Shift = ui::EF_SHIFT_DOWN,
  Control = ui::EF_CONTROL_DOWN,
  Alt = ui::EF_ALT_DOWN
};

#elif defined(UI_WT)
/*enum class KeyCode : int {
  Escape = Wt::Key::Escape,
  Enter = Wt::Key::Enter,
  Delete = Wt::Key::Delete,
  Up = Wt::Key::Up,
  Down = Wt::Key::Down,
};*/
using KeyCode = Wt::Key;
using KeyModifier = Wt::KeyboardModifier;
#endif

#if defined(UI_QT)
#include "controls/qt/key_codes.h"
#endif