#pragma once

#include "aui/qt/message_loop_qt.h"
#include "base/awaitable.h"
#include "base/callback_awaitable.h"

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

// Every awaitable modal helper below has the same contract for a dialog that
// is destroyed while still open -- typically because its parent went first,
// which deletes the child without emitting `accepted`, `rejected` or
// `finished`: the awaiting coroutine chain is ABANDONED. It is destroyed, never
// resumed, and nothing after the `co_await` runs.
//
// That is deliberate. Resuming with a rejection instead would run callers'
// post-dialog code after the widgets it refers to are gone -- e.g.
// `WriteModel::ReportWriteErrorAsync` then calls a completion handler that
// captures the deleted `WriteDialog`'s `this`. The cost is that an outer party
// awaiting the chain's result never receives one, so do not gate shutdown on a
// modal completing. Pinned by the `*AbandonsAwaiterWhenParentDestroyed` cases
// in dialog_util_unittest.cpp.
template <class T, class Mapper>
inline Awaitable<ModalDialogResult<T, Mapper>> RunModalDialogAsync(
    std::unique_ptr<T> dialog,
    Mapper mapper) {
  auto executor = co_await boost::asio::this_coro::executor;
  T* dialog_ptr = dialog.release();

  auto [completion] = co_await CallbackToAwaitable<
      DialogCompletion<ModalDialogResult<T, Mapper>>>(
      executor,
      [dialog_ptr, mapper = std::move(mapper)](auto callback) mutable {
        auto completion = std::make_shared<std::decay_t<decltype(callback)>>(
            std::move(callback));

        QObject::connect(
            dialog_ptr, &QDialog::accepted,
            [dialog_ptr, mapper = std::move(mapper), completion]() mutable {
              try {
                (*completion)(DialogCompletion<ModalDialogResult<T, Mapper>>{
                    .result = mapper(*dialog_ptr)});
              } catch (...) {
                (*completion)(DialogCompletion<ModalDialogResult<T, Mapper>>{
                    .error = std::current_exception()});
              }
              dialog_ptr->deleteLater();
            });

        QObject::connect(
            dialog_ptr, &QDialog::rejected, [dialog_ptr, completion]() mutable {
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
inline Awaitable<ModalDialogResult<T, Mapper>> StartMappedModalDialog(
    std::unique_ptr<T> dialog,
    Mapper mapper) {
  return RunModalDialogAsync(std::move(dialog), std::move(mapper));
}

template <class T>
inline Awaitable<T*> StartModalDialog(std::unique_ptr<T> dialog) {
  return StartMappedModalDialog(std::move(dialog),
                                [](T& dialog) { return &dialog; });
}

// Shows `dialog` as a fire-and-forget modal that deletes itself when finished.
// For callers with no completion to await: the Awaitable-returning
// Start*ModalDialog variants are lazy coroutines, so *discarding* their result
// destroys the never-started frame — and the dialog silently never shows (the
// About / multi-create / print-preview regression).
template <class T>
inline void ShowSelfOwnedModalDialog(std::unique_ptr<T> dialog) {
  T* dialog_ptr = dialog.release();
  QObject::connect(dialog_ptr, &QDialog::finished, dialog_ptr,
                   &QObject::deleteLater);
  dialog_ptr->setModal(true);
  dialog_ptr->show();
}

// Shows `dialog` modally and completes when it is accepted; a rejected dialog
// completes by throwing.
//
// Built on `RunModalDialogAsync` like its siblings so that all of them share
// one answer to a dialog that never finishes (see the note above it). It used
// to wait on an `AsyncCompletion` gate instead, whose waiter list held the
// awaiting frame while the frame held the gate -- so a dialog destroyed with
// its parent leaked that frame and everything it captured.
template <class T>
inline Awaitable<void> StartOwnedModalDialog(std::unique_ptr<T> dialog) {
  co_await RunModalDialogAsync(std::move(dialog), [](T&) { return true; });
  co_return;
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
      executor,
      [dialog_ptr, mapper = std::move(mapper)](auto callback) mutable {
        auto completion = std::make_shared<std::decay_t<decltype(callback)>>(
            std::move(callback));

        QObject::connect(
            dialog_ptr, &QDialog::finished,
            [dialog_ptr, mapper = std::move(mapper),
             completion](int result) mutable {
              try {
                (*completion)(DialogCompletion<FinishedDialogResult<T, Mapper>>{
                    .result = mapper(*dialog_ptr, result)});
              } catch (...) {
                (*completion)(DialogCompletion<FinishedDialogResult<T, Mapper>>{
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
inline Awaitable<FinishedDialogResult<T, Mapper>> StartFinishedModalDialog(
    std::unique_ptr<T> dialog,
    Mapper mapper) {
  return RunFinishedModalDialogAsync(std::move(dialog), std::move(mapper));
}
