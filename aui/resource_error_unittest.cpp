#include "aui/resource_error.h"

#include <gtest/gtest.h>

#include <chrono>
#include <exception>
#include <filesystem>
#include <thread>
#include <vector>

namespace {

using namespace std::chrono_literals;

class RecordingDialogService : public DialogService {
 public:
  UiView* GetDialogOwningWindow() const override { return nullptr; }
  UiView* GetParentWidget() const override { return nullptr; }

  promise<MessageBoxResult> RunMessageBox(std::u16string_view message,
                                          std::u16string_view title,
                                          MessageBoxMode mode) override {
    messages.emplace_back(message);
    titles.emplace_back(title);
    modes.emplace_back(mode);
    message_box_promise = promise<MessageBoxResult>{};
    return message_box_promise;
  }

  promise<std::filesystem::path> SelectOpenFile(
      std::u16string_view title) override {
    return make_rejected_promise<std::filesystem::path>(std::exception{});
  }

  promise<std::filesystem::path> SelectSaveFile(
      const SaveParams& params) override {
    return make_rejected_promise<std::filesystem::path>(std::exception{});
  }

  void CompleteMessageBox(
      MessageBoxResult result = MessageBoxResult::Ok) {
    message_box_promise.resolve(result);
  }

  std::vector<std::u16string> messages;
  std::vector<std::u16string> titles;
  std::vector<MessageBoxMode> modes;
  promise<MessageBoxResult> message_box_promise;
};

template <typename Predicate>
void WaitUntil(Predicate predicate) {
  for (int i = 0; i < 200 && !predicate(); ++i) {
    std::this_thread::sleep_for(1ms);
  }
}

template <typename T>
T WaitForPromise(promise<T> promise) {
  WaitUntil([&] { return promise.wait_for(0ms) != promise_wait_status::timeout; });
  return promise.get();
}

void WaitForPromise(promise<void> promise) {
  WaitUntil([&] { return promise.wait_for(0ms) != promise_wait_status::timeout; });
  promise.get();
}

}  // namespace

TEST(ResourceError, HandleResourceErrorPassesThroughSuccessfulValue) {
  RecordingDialogService dialog_service;

  auto result = HandleResourceError(make_resolved_promise(42), dialog_service,
                                    u"Title");

  EXPECT_EQ(WaitForPromise(std::move(result)), 42);
  EXPECT_TRUE(dialog_service.messages.empty());
}

TEST(ResourceError, HandleResourceErrorShowsDialogAndRethrowsOriginalError) {
  RecordingDialogService dialog_service;
  auto source = make_rejected_promise<int>(ResourceError{u"Load failed"});

  auto result = HandleResourceError(std::move(source), dialog_service,
                                    u"Import");

  WaitUntil([&] { return !dialog_service.messages.empty(); });
  ASSERT_EQ(dialog_service.messages, std::vector<std::u16string>{u"Load failed."});
  ASSERT_EQ(dialog_service.titles, std::vector<std::u16string>{u"Import"});
  ASSERT_EQ(dialog_service.modes, std::vector<MessageBoxMode>{MessageBoxMode::Error});

  dialog_service.CompleteMessageBox();

  EXPECT_THROW(WaitForPromise(std::move(result)), ResourceError);
}

TEST(ResourceError, ShowResourceErrorRethrowsAfterDialogAcknowledged) {
  RecordingDialogService dialog_service;
  auto error = std::make_exception_ptr(ResourceError{u"Save failed"});

  auto result = ShowResourceError<void>(dialog_service, u"Export", error);

  ASSERT_EQ(dialog_service.messages, std::vector<std::u16string>{u"Save failed."});
  ASSERT_EQ(dialog_service.titles, std::vector<std::u16string>{u"Export"});
  dialog_service.CompleteMessageBox();

  EXPECT_THROW(WaitForPromise(std::move(result)), ResourceError);
}
