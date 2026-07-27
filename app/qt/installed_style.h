#pragma once

#include <QApplication>
#include <QSettings>
#include <QString>
#include <QStyle>

// Installs the widget style for the application lifetime and persists an
// explicit operator choice.
//
// The client is a native desktop application (client/docs/ux/principles.md §9),
// so the default is the *platform* style — `windows11`/`windowsvista` on
// Windows, `macos` on macOS, the QT_QPA_PLATFORMTHEME style on Linux. We reach
// that by simply not calling setStyle() at all, which leaves the style Qt
// already picked for the platform.
//
// The `Style` QSetting stays available as an explicit override (the Settings →
// Style menu writes it). It is only persisted when the operator actually chose
// something: writing the resolved platform style name back on every exit would
// pin the client to whatever style shipped with the Qt build it first ran on,
// and it would silently stop following platform-style changes across Qt or OS
// upgrades.
class InstalledStyle {
 public:
  explicit InstalledStyle(QSettings& settings) : settings_{settings} {
    const QString style = settings.value("Style").toString();
    if (!style.isEmpty()) {
      QApplication::setStyle(style);
    }
    installed_ = CurrentStyleName();
  }

  ~InstalledStyle() {
    const QString current = CurrentStyleName();
    // Only record a style the operator switched to at runtime. If nothing
    // changed, leave the setting exactly as we found it (in particular, leave
    // it empty so the platform default keeps winning).
    if (!current.isEmpty() && current != installed_) {
      settings_.setValue("Style", current);
    }
  }

 private:
  static QString CurrentStyleName() {
    const QStyle* style = QApplication::style();
    return style ? style->objectName() : QString{};
  }

  QSettings& settings_;
  QString installed_;
};
