#pragma once

#if defined(UI_QT)
#include <QWidget>
#elif defined(UI_VIEWS)
#include "ui/views/view.h"
#endif

#if defined(UI_QT)
enum class KeyCode : int {
  Escape = Qt::Key_Escape,
  Enter = Qt::Key_Enter,
  Delete = Qt::Key_Delete,
  Up = Qt::Key_Up,
  Down = Qt::Key_Down,
};

#elif defined(UI_VIEWS)
enum class KeyCode : unsigned {
  Escape = ui::KeyboardCode::VKEY_ESCAPE,
  Enter = ui::KeyboardCode::VKEY_RETURN,
  Delete = ui::KeyboardCode::VKEY_DELETE,
  Up = ui::KeyboardCode::VKEY_UP,
  Down = ui::KeyboardCode::VKEY_DOWN,
};
#endif
