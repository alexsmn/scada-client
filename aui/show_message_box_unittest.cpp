#include "aui/show_message_box.h"

#include "base/test/awaitable_test.h"
#include "base/test/test_executor.h"

#include <gtest/gtest.h>

#include <exception>
#include <string>
#include <vector>

namespace {

// Records each box only when the awaitable is actually awaited.
//
// That distinction is the whole point of these tests. `RunMessageBox` is a
// coroutine, so its body — and therefore this recording — does not run when
// the awaitable is merely constructed and dropped. A gmock `EXPECT_CALL` would
// be satisfied by the discarded call and would pass against the pre-fix code;
// this fake fails there, which is what makes it a regression test.
class RecordingDialogService : public DialogService {
 public:
  struct Box {
    std::u16string message;
    std::u16string title;
    MessageBoxMode mode;
  };

  UiView* GetDialogOwningWindow() const override { return nullptr; }
  UiView* GetParentWidget() const override { return nullptr; }

  Awaitable<MessageBoxResult> RunMessageBox(std::u16string_view message,
                                            std::u16string_view title,
                                            MessageBoxMode mode) override {
    boxes.emplace_back(
        Box{std::u16string{message}, std::u16string{title}, mode});
    co_return MessageBoxResult::Ok;
  }

  Awaitable<std::filesystem::path> SelectOpenFile(
      std::u16string_view title) override {
    co_return std::filesystem::path{};
  }

  Awaitable<std::filesystem::path> SelectSaveFile(
      const SaveParams& params) override {
    co_return std::filesystem::path{};
  }

  std::vector<Box> boxes;
};

// Fails the way a real dialog service can when the UI is being torn down.
class ThrowingDialogService : public RecordingDialogService {
 public:
  Awaitable<MessageBoxResult> RunMessageBox(std::u16string_view message,
                                            std::u16string_view title,
                                            MessageBoxMode mode) override {
    throw std::runtime_error{"dialog service is gone"};
    co_return MessageBoxResult::Ok;
  }
};

class ShowMessageBoxTest : public testing::Test {
 protected:
  void Drain() { ::Drain(executor_); }

  TestExecutor executor_;
  RecordingDialogService dialog_service_;
};

// Regression: the seven call sites of task 343 called `RunMessageBox` bare.
// It returns a lazy awaitable, so the discarded coroutine never started and no
// box was ever shown — the operator was told nothing at all.
TEST_F(ShowMessageBoxTest, RunsTheDiscardedAwaitable) {
  ShowMessageBox(executor_, dialog_service_, u"Invalid expression.",
                 /*title=*/{}, MessageBoxMode::Error);
  Drain();

  ASSERT_EQ(dialog_service_.boxes.size(), 1u);
  EXPECT_EQ(dialog_service_.boxes[0].message, u"Invalid expression.");
  EXPECT_EQ(dialog_service_.boxes[0].mode, MessageBoxMode::Error);
}

TEST_F(ShowMessageBoxTest, ForwardsTitleAndMode) {
  ShowMessageBox(executor_, dialog_service_, u"No data to export.", u"Export",
                 MessageBoxMode::Info);
  Drain();

  ASSERT_EQ(dialog_service_.boxes.size(), 1u);
  EXPECT_EQ(dialog_service_.boxes[0].message, u"No data to export.");
  EXPECT_EQ(dialog_service_.boxes[0].title, u"Export");
  EXPECT_EQ(dialog_service_.boxes[0].mode, MessageBoxMode::Info);
}

// The box outlives the call that asked for it, and `RunMessageBox` takes
// views. Taking the arguments by value is what keeps this from reading freed
// memory once the caller's temporary is gone.
TEST_F(ShowMessageBoxTest, OwnsItsMessageAfterTheCallerTemporaryDies) {
  {
    std::u16string message = u"Export error.";
    std::u16string title = u"Export";
    ShowMessageBox(executor_, dialog_service_, std::move(message),
                   std::move(title), MessageBoxMode::Error);
  }
  Drain();

  ASSERT_EQ(dialog_service_.boxes.size(), 1u);
  EXPECT_EQ(dialog_service_.boxes[0].message, u"Export error.");
  EXPECT_EQ(dialog_service_.boxes[0].title, u"Export");
}

// The coroutine is spawned detached, and an exception escaping a detached
// coroutine calls std::terminate. A failing dialog service must not take the
// client down with it.
TEST_F(ShowMessageBoxTest, SwallowsExceptionsFromTheDialogService) {
  ThrowingDialogService throwing_dialog_service;

  ShowMessageBox(executor_, throwing_dialog_service, u"Export error.",
                 u"Export", MessageBoxMode::Error);
  Drain();

  EXPECT_TRUE(throwing_dialog_service.boxes.empty());
}

// Nothing is shown until the executor runs, so a caller that spawns during
// teardown does not get a box drawn from under it.
TEST_F(ShowMessageBoxTest, DoesNothingBeforeTheExecutorRuns) {
  ShowMessageBox(executor_, dialog_service_, u"Invalid expression.",
                 /*title=*/{}, MessageBoxMode::Error);

  EXPECT_TRUE(dialog_service_.boxes.empty());
}

}  // namespace
