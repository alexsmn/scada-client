#include "aui/qt/color_role_qt.h"

#include "aui/severity_colors.h"

#include <QApplication>
#include <QColor>
#include <QIcon>
#include <QPalette>
#include <QStyle>

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
    case ColorRole::Warning:
      // One severity palette system-wide (principles §5): the same warning
      // text colour the Inspector's limit badge takes, following the theme --
      // the text form, since a cell is text on the base (backlog 1144).
      return SeverityTextColor(SeverityLevel::kWarning)->qcolor();
    case ColorRole::Default:
      return QVariant{};
  }
  return QVariant{};
}

QVariant ColorRoleDecoration(ColorRole role) {
  if (role != ColorRole::Warning)
    return QVariant{};
  return QApplication::style()->standardIcon(QStyle::SP_MessageBoxWarning);
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
    case ColorRole::Warning:
    case ColorRole::Default:
      return QVariant{};
  }
  return QVariant{};
}

}  // namespace scada::aui
