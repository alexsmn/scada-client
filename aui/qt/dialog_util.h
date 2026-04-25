#pragma once

#include "aui/qt/message_loop_qt.h"
#include "base/awaitable.h"
#include "base/awaitable_promise.h"
#include "base/promise.h"
#include "net/net_executor_adapter.h"

#include <QDialog>

#include <exception>
#include <memory>
#include <type_traits>
#include <utility>

template <class T, class Mapper>
using ModalDialogResult = std::invoke_result_t<Mapper&, T&>;

template <class T, class Mapper>
inline Awaitable<ModalDialogResult<T, Mapper>> RunModalDialogAsync(
    std::unique_ptr<T> dialog,
    Mapper mapper) {
  auto executor = co_await boost::asio::this_coro::executor;
  promise<ModalDialogResult<T, Mapper>> promise;

  QObject::connect(dialog.get(), &QDialog::accepted,
                   [&dialog = *dialog, mapper = std::move(mapper),
                    promise]() mutable {
                     try {
                       promise.resolve(mapper(dialog));
                     } catch (...) {
                       promise.reject(std::current_exception());
                     }
                     dialog.deleteLater();
                   });

  QObject::connect(dialog.get(), &QDialog::rejected,
                   [&dialog = *dialog, promise]() mutable {
                     promise.reject(std::exception{});
                     dialog.deleteLater();
                   });

  dialog->setModal(true);
  dialog.release()->show();

  co_return co_await AwaitPromise(executor, std::move(promise));
}

template <class T, class Mapper>
inline promise<ModalDialogResult<T, Mapper>> StartMappedModalDialog(
    std::unique_ptr<T> dialog,
    Mapper mapper) {
  auto executor = std::make_shared<MessageLoopQt>();
  return ToPromise(NetExecutorAdapter{executor},
                   RunModalDialogAsync(std::move(dialog), std::move(mapper)));
}

template <class T>
inline promise<T*> StartModalDialog(std::unique_ptr<T> dialog) {
  return StartMappedModalDialog(std::move(dialog),
                                [](T& dialog) { return &dialog; });
}

template <class T, class Mapper>
using FinishedDialogResult = std::invoke_result_t<Mapper&, T&, int>;

template <class T, class Mapper>
inline Awaitable<FinishedDialogResult<T, Mapper>> RunFinishedModalDialogAsync(
    std::unique_ptr<T> dialog,
    Mapper mapper) {
  auto executor = co_await boost::asio::this_coro::executor;
  promise<FinishedDialogResult<T, Mapper>> promise;

  QObject::connect(dialog.get(), &QDialog::finished,
                   [&dialog = *dialog, mapper = std::move(mapper),
                    promise](int result) mutable {
                     try {
                       promise.resolve(mapper(dialog, result));
                     } catch (...) {
                       promise.reject(std::current_exception());
                     }
                     dialog.deleteLater();
                   });

  dialog->setModal(true);
  dialog.release()->show();

  co_return co_await AwaitPromise(executor, std::move(promise));
}

template <class T, class Mapper>
inline promise<FinishedDialogResult<T, Mapper>> StartFinishedModalDialog(
    std::unique_ptr<T> dialog,
    Mapper mapper) {
  auto executor = std::make_shared<MessageLoopQt>();
  return ToPromise(
      NetExecutorAdapter{executor},
      RunFinishedModalDialogAsync(std::move(dialog), std::move(mapper)));
}
