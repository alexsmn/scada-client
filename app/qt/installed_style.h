#pragma once

#include <QApplication>
#include <QSettings>
#include <QStyle>

namespace {
const char kDefaultStyle[] = "Fusion";
}

class InstalledStyle {
 public:
  explicit InstalledStyle(QSettings& settings) : settings_{settings} {
    auto style = settings.value("Style").toString();
    QApplication::setStyle(style.isEmpty() ? kDefaultStyle : style);
  }

  ~InstalledStyle() {
    if (const auto* style = QApplication::style()) {
      settings_.setValue("Style", style->objectName());
    }
  }

 private:
  QSettings& settings_;
};
