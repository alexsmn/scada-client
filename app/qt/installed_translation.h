#pragma once

#include "base/boost_log.h"
#include "base/program_options.h"

#include <QApplication>
#include <QLibraryInfo>
#include <QSettings>
#include <QTranslator>

class InstalledTranslation {
 public:
  explicit InstalledTranslation(QSettings& settings) : settings_{settings} {
    auto locale_name = GetLocaleName();

    const auto local_translation_dir =
        QApplication::applicationDirPath() + "/translations";

    const auto global_translation_dir =
        QLibraryInfo::path(QLibraryInfo::TranslationsPath);

    // Qt's own catalogs must be installed before the client one: QTranslator
    // lookup walks translators in reverse installation order, so the client
    // catalog wins on conflicts. `qtbase_*` is loaded explicitly (standard
    // QMessageBox buttons, print-preview chrome, etc.); the `qt_*`
    // meta-catalog only pulls it in when its whole dependency set resolves,
    // which fails silently on partial Qt deployments. Prefer Qt's installed
    // translations dir and fall back to the app's staged `translations/` for
    // deployed installs where Qt's own dir is absent.
    const auto qtbase_translation_name = "qtbase_" + locale_name;
    if (qtbase_translator_.load(qtbase_translation_name,
                                global_translation_dir) ||
        qtbase_translator_.load(qtbase_translation_name,
                                local_translation_dir)) {
      QApplication::installTranslator(&qtbase_translator_);
    } else {
      // Not fatal — a user running an unsupported locale legitimately has no
      // catalog. It is logged because the failure is otherwise invisible: the
      // app keeps running and renders translated client strings over English
      // standard buttons, which reads as a UI bug rather than a missing file.
      BOOST_LOG_TRIVIAL(warning)
          << "Qt base catalog " << qtbase_translation_name.toStdString()
          << ".qm not found in " << global_translation_dir.toStdString()
          << " or " << local_translation_dir.toStdString()
          << "; standard Qt chrome will render in English.";
    }

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

    if (auto locale_name = client::GetOptionValue("locale");
        !locale_name.empty()) {
      return QString::fromStdString(locale_name);
    }

    return QLocale::system().bcp47Name();
  }

  QSettings& settings_;

  QTranslator qtbase_translator_;
  QTranslator qt_translator_;
  QTranslator app_translator_;
};
