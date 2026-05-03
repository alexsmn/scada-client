#pragma once

#include "base/any_executor.h"

#include "aui/qt/message_loop_qt.h"
#include "base/awaitable.h"

#include <QApplication>
#include <QDialog>
#include <QEventLoop>

#include <chrono>
#include <exception>
#include <memory>
#include <optional>
#include <thread>
#include <type_traits>

namespace aui::qt::test {

template <class T>
struct AwaitableResult {
  std::optional<T> value;
  std::exception_ptr error;
  bool done = false;
};

template <>
struct AwaitableResult<void> {
  std::exception_ptr error;
  bool done = false;
};

template <class T>
std::shared_ptr<AwaitableResult<T>> StartAwaitable(Awaitable<T> awaitable) {
  auto executor = MakeAnyExecutor(std::make_shared<MessageLoopQt>());
  auto result = std::make_shared<AwaitableResult<T>>();
  CoSpawn(executor, [result, awaitable = std::move(awaitable)]() mutable
                       -> Awaitable<void> {
    try {
      if constexpr (std::is_void_v<T>) {
        co_await std::move(awaitable);
      } else {
        result->value.emplace(co_await std::move(awaitable));
      }
    } catch (...) {
      result->error = std::current_exception();
    }
    result->done = true;
  });
  return result;
}

template <class T>
bool IsAwaitableReady(const std::shared_ptr<AwaitableResult<T>>& result) {
  return result->done;
}

template <class T>
T GetAwaitableResult(const std::shared_ptr<AwaitableResult<T>>& result) {
  if (result->error) {
    std::rethrow_exception(result->error);
  }
  return std::move(*result->value);
}

inline void GetAwaitableResult(
    const std::shared_ptr<AwaitableResult<void>>& result) {
  if (result->error) {
    std::rethrow_exception(result->error);
  }
}

template <class T, class DialogAction>
void ProcessEventsUntilSettled(const std::shared_ptr<AwaitableResult<T>>& result,
                               DialogAction action) {
  bool acted = false;
  for (int i = 0; i < 200 && !IsAwaitableReady(result); ++i) {
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
