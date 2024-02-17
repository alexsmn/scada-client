#pragma once

#include "base/command_line.h"

#include <QApplication>
#include <QLibraryInfo>
#include <QSettings>
#include <QTranslator>

class InstalledTranslation {
 public:
  InstalledTranslation(QSettings& settings) : settings_{settings} {
    auto locale_name = GetLocaleName();

    const auto local_translation_dir =
        QApplication::applicationDirPath() + "/translations";

    const auto global_translation_dir =
        QLibraryInfo::location(QLibraryInfo::TranslationsPath);

    const auto qt_translation_name = "qt_" + locale_name;
    if (qt_translator_.load(qt_translation_name, local_translation_dir) ||
        qt_translator_.load(qt_translation_name, global_translation_dir)) {
      QApplication::installTranslator(&qt_translator_);
    }

    const auto client_translation_name = "client_" + locale_name;
    if (app_translator_.load(client_translation_name, local_translation_dir)) {
      QApplication::installTranslator(&app_translator_);
    }
  }

 private:
  QString GetLocaleName() const {
    if (auto locale_name = settings_.value("LocaleName").toString();
        !locale_name.isEmpty()) {
      return locale_name;
    }

    if (auto locale_name =
            base::CommandLine::ForCurrentProcess()->GetSwitchValueASCII(
                "locale");
        !locale_name.empty()) {
      return QString::fromStdString(locale_name);
    }

    return QLocale::system().name();
  }

  QSettings& settings_;

  QTranslator qt_translator_;
  QTranslator app_translator_;
};
