#pragma once

#include "aui/qt/message_loop_qt.h"
#include "base/async_completion.h"
#include "base/awaitable.h"
#include "base/awaitable_promise.h"
#include "base/callback_awaitable.h"
#include "base/promise.h"
#include "net/net_executor_adapter.h"

#include <QDialog>

#include <exception>
#include <memory>
#include <optional>
#include <type_traits>
#include <utility>

template <class T, class Mapper>
using ModalDialogResult = std::invoke_result_t<Mapper&, T&>;

template <class T>
struct DialogCompletion {
  std::exception_ptr error;
  std::optional<T> result;
};

template <class T, class Mapper>
inline Awaitable<ModalDialogResult<T, Mapper>> RunModalDialogAsync(
    std::unique_ptr<T> dialog,
    Mapper mapper) {
  auto executor = co_await boost::asio::this_coro::executor;
  T* dialog_ptr = dialog.release();

  auto [completion] =
      co_await CallbackToAwaitable<DialogCompletion<ModalDialogResult<T, Mapper>>>(
          executor,
          [dialog_ptr, mapper = std::move(mapper)](auto callback) mutable {
            auto completion =
                std::make_shared<std::decay_t<decltype(callback)>>(
                    std::move(callback));

            QObject::connect(
                dialog_ptr, &QDialog::accepted,
                [dialog_ptr, mapper = std::move(mapper),
                 completion]() mutable {
                  try {
                    (*completion)(
                        DialogCompletion<ModalDialogResult<T, Mapper>>{
                            .result = mapper(*dialog_ptr)});
                  } catch (...) {
                    (*completion)(
                        DialogCompletion<ModalDialogResult<T, Mapper>>{
                            .error = std::current_exception()});
                  }
                  dialog_ptr->deleteLater();
                });

            QObject::connect(
                dialog_ptr, &QDialog::rejected,
                [dialog_ptr, completion]() mutable {
                  (*completion)(DialogCompletion<ModalDialogResult<T, Mapper>>{
                      .error = std::make_exception_ptr(std::exception{})});
                  dialog_ptr->deleteLater();
                });

            dialog_ptr->setModal(true);
            dialog_ptr->show();
          });

  if (completion.error) {
    std::rethrow_exception(completion.error);
  }
  co_return std::move(*completion.result);
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

template <class T>
inline promise<void> StartOwnedModalDialog(std::unique_ptr<T> dialog) {
  auto executor = std::make_shared<MessageLoopQt>();
  base::AsyncCompletion completion{NetExecutorAdapter{executor}};
  T* dialog_ptr = dialog.release();

  QObject::connect(dialog_ptr, &QDialog::accepted,
                   [dialog_ptr, completion]() mutable {
                     completion.Complete();
                     dialog_ptr->deleteLater();
                   });

  QObject::connect(dialog_ptr, &QDialog::rejected,
                   [dialog_ptr, completion]() mutable {
                     completion.Fail(std::make_exception_ptr(std::exception{}));
                     dialog_ptr->deleteLater();
                   });

  dialog_ptr->setModal(true);
  dialog_ptr->show();

  return ToPromise(NetExecutorAdapter{executor}, completion.Wait());
}

template <class T, class Mapper>
using FinishedDialogResult = std::invoke_result_t<Mapper&, T&, int>;

template <class T, class Mapper>
inline Awaitable<FinishedDialogResult<T, Mapper>> RunFinishedModalDialogAsync(
    std::unique_ptr<T> dialog,
    Mapper mapper) {
  auto executor = co_await boost::asio::this_coro::executor;
  T* dialog_ptr = dialog.release();

  auto [completion] = co_await CallbackToAwaitable<
      DialogCompletion<FinishedDialogResult<T, Mapper>>>(
      executor, [dialog_ptr, mapper = std::move(mapper)](auto callback) mutable {
        auto completion =
            std::make_shared<std::decay_t<decltype(callback)>>(
                std::move(callback));

        QObject::connect(
            dialog_ptr, &QDialog::finished,
            [dialog_ptr, mapper = std::move(mapper),
             completion](int result) mutable {
              try {
                (*completion)(
                    DialogCompletion<FinishedDialogResult<T, Mapper>>{
                        .result = mapper(*dialog_ptr, result)});
              } catch (...) {
                (*completion)(
                    DialogCompletion<FinishedDialogResult<T, Mapper>>{
                        .error = std::current_exception()});
              }
              dialog_ptr->deleteLater();
            });

        dialog_ptr->setModal(true);
        dialog_ptr->show();
      });

  if (completion.error) {
    std::rethrow_exception(completion.error);
  }
  co_return std::move(*completion.result);
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
