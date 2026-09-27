#include "aui/qt/color_role_qt.h"

#include <QApplication>
#include <QColor>
#include <QPalette>

namespace scada::aui {

QVariant ColorRoleForeground(ColorRole role) {
  const QPalette& palette = QApplication::palette();
  switch (role) {
    case ColorRole::Disabled:
      return palette.color(QPalette::Disabled, QPalette::Text);
    case ColorRole::Header:
      return palette.color(QPalette::Normal, QPalette::ButtonText);
    case ColorRole::Placeholder:
      return palette.color(QPalette::Normal, QPalette::PlaceholderText);
    case ColorRole::Default:
      return QVariant{};
  }
  return QVariant{};
}

QVariant ColorRoleBackground(ColorRole role) {
  const QPalette& palette = QApplication::palette();
  switch (role) {
    case ColorRole::Header:
      return palette.color(QPalette::Normal, QPalette::Button);
    // A disabled cell is greyed by its text colour alone; tinting the row
    // behind it as well would read as a selection.
    case ColorRole::Disabled:
    case ColorRole::Placeholder:
    case ColorRole::Default:
      return QVariant{};
  }
  return QVariant{};
}

}  // namespace scada::aui
