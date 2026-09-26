#include "aui/qt/dialog_util.h"

#include "aui/qt/dialog_test_util.h"
#include "aui/test/app_environment.h"

#include <QApplication>
#include <QDialog>
#include <QEventLoop>
#include <QPointer>
#include <QWidget>
#include <gtest/gtest.h>

#include <chrono>
#include <stdexcept>
#include <thread>

namespace {

class DialogUtilTest : public testing::Test {
 protected:
  AppEnvironment app_env_;
};

template <class T>
Awaitable<T> MakeResolvedAwaitable(T value) {
  co_return std::move(value);
}

template <typename Predicate>
void ProcessEventsUntil(Predicate predicate) {
  for (int i = 0; i < 200 && !predicate(); ++i) {
    QApplication::processEvents(
        QEventLoop::AllEvents | QEventLoop::WaitForMoreEvents, 20);
    std::this_thread::sleep_for(std::chrono::milliseconds{1});
  }
}

// Starts `awaitable` on a modal dialog parented to a widget, destroys the
// parent while the dialog is still open, and reports what became of the
// awaiting coroutine: whether it was resumed, and whether its frame was
// released. A parent destroyed first deletes its child dialog without emitting
// `accepted`, `rejected` or `finished` -- which is how ⌘Q, or a main window
// closing, takes a modal down.
struct ParentDestroyedOutcome {
  bool resumed = false;
  bool frame_released = false;
};

template <class T, class StartFn>
ParentDestroyedOutcome DestroyParentOfOpenDialog(StartFn start) {
  auto parent = std::make_unique<QWidget>();
  auto dialog = std::make_unique<QDialog>(parent.get());
  QPointer<QDialog> tracker{dialog.get()};

  auto result =
      scada::aui::qt::test::StartAwaitable<T>(start(std::move(dialog)));
  ProcessEventsUntil([&] { return tracker && tracker->isVisible(); });
  EXPECT_TRUE(tracker && tracker->isVisible());

  parent.reset();
  EXPECT_TRUE(tracker.isNull());

  // `result` is shared with the spawned coroutine's frame, so its use count
  // drops to one exactly when that frame is destroyed. Frame destruction is
  // posted through the executor, hence the drain.
  ProcessEventsUntil([&] { return result->done || result.use_count() == 1; });
  QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
  ProcessEventsUntil([&] { return result.use_count() == 1; });

  return {.resumed = result->done, .frame_released = result.use_count() == 1};
}

}  // namespace

TEST_F(DialogUtilTest, DialogTestUtilDoesNotInvokeActionForSettledPromise) {
  auto result = scada::aui::qt::test::StartAwaitable(MakeResolvedAwaitable(7));
  int action_count = 0;

  scada::aui::qt::test::ProcessEventsUntilSettled(
      result, [&](QDialog&) { ++action_count; });

  EXPECT_TRUE(scada::aui::qt::test::IsAwaitableReady(result));
  EXPECT_EQ(scada::aui::qt::test::GetAwaitableResult(result), 7);
  EXPECT_EQ(action_count, 0);
}

// Regression: the Awaitable-returning Start*ModalDialog variants are lazy
// coroutines, so a fire-and-forget caller that discarded the result destroyed
// the never-started frame and its dialog silently never showed (the About /
// multi-create / print-preview bug). ShowSelfOwnedModalDialog shows eagerly
// and self-deletes when finished.
TEST_F(DialogUtilTest, ShowSelfOwnedModalDialogShowsAndSelfDeletes) {
  auto dialog = std::make_unique<QDialog>();
  QPointer<QDialog> tracker{dialog.get()};

  ShowSelfOwnedModalDialog(std::move(dialog));

  ASSERT_FALSE(tracker.isNull());
  EXPECT_TRUE(tracker->isVisible());
  EXPECT_TRUE(tracker->isModal());

  tracker->reject();
  // deleteLater is a DeferredDelete event, which processEvents leaves queued;
  // flush it explicitly.
  QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
  EXPECT_TRUE(tracker.isNull());
}

TEST_F(DialogUtilTest, StartMappedModalDialogReturnsAcceptedMappedResult) {
  auto result = scada::aui::qt::test::StartAwaitable(StartMappedModalDialog(
      std::make_unique<QDialog>(),
      [](QDialog& dialog) { return dialog.isModal() ? 42 : 0; }));

  scada::aui::qt::test::ProcessEventsUntilSettled(
      result, scada::aui::qt::test::AcceptDialog);

  ASSERT_TRUE(scada::aui::qt::test::IsAwaitableReady(result));
  EXPECT_EQ(scada::aui::qt::test::GetAwaitableResult(result), 42);
}

TEST_F(DialogUtilTest, StartOwnedModalDialogShowsDialogUntilAccepted) {
  auto dialog = std::make_unique<QDialog>();
  QPointer<QDialog> dialog_ptr = dialog.get();

  auto result = scada::aui::qt::test::StartAwaitable(
      StartOwnedModalDialog(std::move(dialog)));

  ProcessEventsUntil([&] { return dialog_ptr && dialog_ptr->isVisible(); });
  ASSERT_TRUE(dialog_ptr);
  EXPECT_TRUE(dialog_ptr->isModal());
  EXPECT_TRUE(dialog_ptr->isVisible());

  scada::aui::qt::test::AcceptDialog(*dialog_ptr);
  scada::aui::qt::test::ProcessEventsUntilSettled(result, [](QDialog&) {});

  ASSERT_TRUE(scada::aui::qt::test::IsAwaitableReady(result));
  EXPECT_NO_THROW(scada::aui::qt::test::GetAwaitableResult(result));
}

TEST_F(DialogUtilTest, StartOwnedModalDialogRejectsCanceledDialog) {
  auto dialog = std::make_unique<QDialog>();
  QPointer<QDialog> dialog_ptr = dialog.get();

  auto result = scada::aui::qt::test::StartAwaitable(
      StartOwnedModalDialog(std::move(dialog)));

  ProcessEventsUntil([&] { return dialog_ptr && dialog_ptr->isVisible(); });
  ASSERT_TRUE(dialog_ptr);
  EXPECT_TRUE(dialog_ptr->isVisible());

  scada::aui::qt::test::RejectDialog(*dialog_ptr);
  scada::aui::qt::test::ProcessEventsUntilSettled(result, [](QDialog&) {});

  ASSERT_TRUE(scada::aui::qt::test::IsAwaitableReady(result));
  EXPECT_THROW(scada::aui::qt::test::GetAwaitableResult(result),
               std::exception);
}

TEST_F(DialogUtilTest, StartMappedModalDialogRejectsCanceledDialog) {
  auto result = scada::aui::qt::test::StartAwaitable(StartMappedModalDialog(
      std::make_unique<QDialog>(), [](QDialog& dialog) { return 42; }));

  scada::aui::qt::test::ProcessEventsUntilSettled(
      result, scada::aui::qt::test::RejectDialog);

  ASSERT_TRUE(scada::aui::qt::test::IsAwaitableReady(result));
  EXPECT_THROW(scada::aui::qt::test::GetAwaitableResult(result),
               std::exception);
}

TEST_F(DialogUtilTest, StartMappedModalDialogRejectsMapperException) {
  auto result = scada::aui::qt::test::StartAwaitable(StartMappedModalDialog(
      std::make_unique<QDialog>(),
      [](QDialog& dialog) -> int { throw std::runtime_error{"map"}; }));

  scada::aui::qt::test::ProcessEventsUntilSettled(
      result, scada::aui::qt::test::AcceptDialog);

  ASSERT_TRUE(scada::aui::qt::test::IsAwaitableReady(result));
  EXPECT_THROW(scada::aui::qt::test::GetAwaitableResult(result),
               std::runtime_error);
}

TEST_F(DialogUtilTest, StartFinishedModalDialogReturnsMappedResult) {
  auto result = scada::aui::qt::test::StartAwaitable(StartFinishedModalDialog(
      std::make_unique<QDialog>(), [](QDialog& dialog, int finished_result) {
        return dialog.isModal() ? finished_result : 0;
      }));

  scada::aui::qt::test::ProcessEventsUntilSettled(
      result, scada::aui::qt::test::AcceptDialog);

  ASSERT_TRUE(scada::aui::qt::test::IsAwaitableReady(result));
  EXPECT_EQ(scada::aui::qt::test::GetAwaitableResult(result),
            QDialog::Accepted);
}

TEST_F(DialogUtilTest, StartFinishedModalDialogRejectsMapperException) {
  auto result = scada::aui::qt::test::StartAwaitable(
      StartFinishedModalDialog(std::make_unique<QDialog>(),
                               [](QDialog& dialog, int finished_result) -> int {
                                 throw std::runtime_error{"map"};
                               }));

  scada::aui::qt::test::ProcessEventsUntilSettled(
      result, scada::aui::qt::test::RejectDialog);

  ASSERT_TRUE(scada::aui::qt::test::IsAwaitableReady(result));
  EXPECT_THROW(scada::aui::qt::test::GetAwaitableResult(result),
               std::runtime_error);
}

// Regression for 727. A dialog whose parent is destroyed while it is open
// finishes neither way, so the awaiter is ABANDONED: its coroutine chain is
// destroyed without being resumed. That is the contract the header states, and
// it is deliberate -- resuming with a rejection would run the caller's
// post-dialog code (WriteModel::ReportWriteErrorAsync calls a completion
// handler capturing the since-deleted WriteDialog's `this`) after the widgets
// it refers to are gone. `StartOwnedModalDialog` used to wait on an
// `AsyncCompletion` gate whose waiter list held the awaiting frame while the
// frame held the gate, so the frame leaked with everything it captured.
TEST_F(DialogUtilTest, OwnedModalDialogAbandonsAwaiterWhenParentDestroyed) {
  auto outcome =
      DestroyParentOfOpenDialog<void>([](std::unique_ptr<QDialog> dialog) {
        return StartOwnedModalDialog(std::move(dialog));
      });
  EXPECT_FALSE(outcome.resumed);
  EXPECT_TRUE(outcome.frame_released);
}

TEST_F(DialogUtilTest, MappedModalDialogAbandonsAwaiterWhenParentDestroyed) {
  auto outcome =
      DestroyParentOfOpenDialog<int>([](std::unique_ptr<QDialog> dialog) {
        return StartMappedModalDialog(std::move(dialog),
                                      [](QDialog&) { return 42; });
      });
  EXPECT_FALSE(outcome.resumed);
  EXPECT_TRUE(outcome.frame_released);
}

TEST_F(DialogUtilTest, FinishedModalDialogAbandonsAwaiterWhenParentDestroyed) {
  auto outcome =
      DestroyParentOfOpenDialog<int>([](std::unique_ptr<QDialog> dialog) {
        return StartFinishedModalDialog(
            std::move(dialog), [](QDialog&, int result) { return result; });
      });
  EXPECT_FALSE(outcome.resumed);
  EXPECT_TRUE(outcome.frame_released);
}
