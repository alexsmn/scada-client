#pragma once

#include "base/promise.h"

#include <QDialog>

template <class T>
inline promise<T*> StartModalDialog(std::unique_ptr<T> dialog) {
  promise<T*> promise;

  QObject::connect(dialog.get(), &QDialog::accepted,
                   [&dialog = *dialog, promise]() mutable {
                     promise.resolve(&dialog);
                     dialog.deleteLater();
                   });

  QObject::connect(dialog.get(), &QDialog::rejected,
                   [&dialog = *dialog, promise]() mutable {
                     promise.reject(std::exception{});
                     dialog.deleteLater();
                   });

  dialog->setModal(true);
  dialog.release()->show();

  return promise;
}
