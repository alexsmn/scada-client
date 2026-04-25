#pragma once

#include "base/promise.h"

#include <QApplication>
#include <QDialog>
#include <QEventLoop>

#include <chrono>
#include <thread>

namespace aui::qt::test {

template <class T>
bool IsPromiseReady(promise<T>& result) {
  return result.wait_for(std::chrono::milliseconds{0}) !=
         promise_wait_status::timeout;
}

template <class T, class DialogAction>
void ProcessEventsUntilSettled(promise<T>& result, DialogAction action) {
  bool acted = false;
  for (int i = 0; i < 200 && !IsPromiseReady(result); ++i) {
    QApplication::processEvents(QEventLoop::AllEvents |
                                    QEventLoop::WaitForMoreEvents,
                                20);
    if (!acted) {
      if (auto* dialog =
              qobject_cast<QDialog*>(QApplication::activeModalWidget())) {
        action(*dialog);
        acted = true;
      }
    }
    std::this_thread::sleep_for(std::chrono::milliseconds{1});
  }
}

inline void AcceptDialog(QDialog& dialog) {
  dialog.accept();
}

inline void RejectDialog(QDialog& dialog) {
  dialog.reject();
}

}  // namespace aui::qt::test
