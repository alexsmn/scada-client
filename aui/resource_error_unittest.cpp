#include "aui/resource_error.h"

#include "base/async_completion.h"
#include "base/test/awaitable_test.h"

#include <gtest/gtest.h>

#include <exception>
#include <filesystem>
#include <memory>
#include <thread>
#include <vector>

namespace {

template <class T>
Awaitable<T> MakeResolvedAwaitable(T value) {
  co_return std::move(value);
}

template <class T>
Awaitable<T> MakeRejectedAwaitable(std::exception_ptr error) {
  std::rethrow_exception(std::move(error));
  co_return T{};
}

class RecordingDialogService : public DialogService {
 public:
  explicit RecordingDialogService(AnyExecutor executor)
      : executor_{std::move(executor)} {}

  UiView* GetDialogOwningWindow() const override { return nullptr; }
  UiView* GetParentWidget() const override { return nullptr; }

  Awaitable<MessageBoxResult> RunMessageBox(std::u16string_view message,
                                            std::u16string_view title,
                                            MessageBoxMode mode) override {
    messages.emplace_back(message);
    titles.emplace_back(title);
    modes.emplace_back(mode);
    message_box_completion = std::make_unique<base::AsyncCompletion>(executor_);
    co_await message_box_completion->Wait();
    co_return message_box_result;
  }

  Awaitable<std::filesystem::path> SelectOpenFile(
      std::u16string_view title) override {
    return MakeRejectedAwaitable<std::filesystem::path>(
        std::make_exception_ptr(std::exception{}));
  }

  Awaitable<std::filesystem::path> SelectSaveFile(
      const SaveParams& params) override {
    return MakeRejectedAwaitable<std::filesystem::path>(
        std::make_exception_ptr(std::exception{}));
  }

  void CompleteMessageBox(
      MessageBoxResult result = MessageBoxResult::Ok) {
    message_box_result = result;
    message_box_completion->Complete();
  }

 private:
  AnyExecutor executor_;
  MessageBoxResult message_box_result = MessageBoxResult::Ok;
  std::unique_ptr<base::AsyncCompletion> message_box_completion;

 public:
  std::vector<std::u16string> messages;
  std::vector<std::u16string> titles;
  std::vector<MessageBoxMode> modes;
};

template <typename Predicate>
void WaitUntil(TestExecutor executor, Predicate predicate) {
  for (int i = 0; i < 200 && !predicate(); ++i) {
    Drain(executor);
    std::this_thread::yield();
  }
}

}  // namespace

TEST(ResourceError, HandleResourceErrorPassesThroughSuccessfulValue) {
  TestExecutor executor;
  RecordingDialogService dialog_service{executor};

  auto result = HandleResourceError(MakeResolvedAwaitable(42), dialog_service,
                                    u"Title");

  EXPECT_EQ(WaitAwaitable(executor, std::move(result)), 42);
  EXPECT_TRUE(dialog_service.messages.empty());
}

TEST(ResourceError, HandleResourceErrorShowsDialogAndRethrowsOriginalError) {
  TestExecutor executor;
  RecordingDialogService dialog_service{executor};
  auto source = MakeRejectedAwaitable<int>(
      std::make_exception_ptr(ResourceError{u"Load failed"}));

  auto result = HandleResourceError(std::move(source), dialog_service,
                                    u"Import");
  auto result_state = StartAwaitable(executor, std::move(result));

  WaitUntil(executor, [&] { return !dialog_service.messages.empty(); });
  ASSERT_EQ(dialog_service.messages, std::vector<std::u16string>{u"Load failed."});
  ASSERT_EQ(dialog_service.titles, std::vector<std::u16string>{u"Import"});
  ASSERT_EQ(dialog_service.modes, std::vector<MessageBoxMode>{MessageBoxMode::Error});

  dialog_service.CompleteMessageBox();

  EXPECT_THROW(WaitResult(executor, result_state), ResourceError);
}

TEST(ResourceError, ShowResourceErrorRethrowsAfterDialogAcknowledged) {
  TestExecutor executor;
  RecordingDialogService dialog_service{executor};
  auto error = std::make_exception_ptr(ResourceError{u"Save failed"});

  auto result = ShowResourceError<void>(dialog_service, u"Export", error);
  auto result_state = StartAwaitable(executor, std::move(result));
  WaitUntil(executor, [&] { return !dialog_service.messages.empty(); });

  ASSERT_EQ(dialog_service.messages, std::vector<std::u16string>{u"Save failed."});
  ASSERT_EQ(dialog_service.titles, std::vector<std::u16string>{u"Export"});
  dialog_service.CompleteMessageBox();

  EXPECT_THROW(WaitResult(executor, result_state), ResourceError);
}
