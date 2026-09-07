#pragma once

#include "aui/dialog_service.h"

#include <gmock/gmock.h>

class MockDialogService : public DialogService {
 public:
  MockDialogService() {
    using namespace testing;

    ON_CALL(*this, SelectOpenFile(/*title=*/_))
        .WillByDefault([](std::u16string_view)
                           -> Awaitable<std::filesystem::path> {
          throw std::exception{};
        });

    // Every `Awaitable`-returning method needs a default action, or gmock
    // answers an unstubbed call with a default-constructed
    // `boost::asio::awaitable` — a null frame whose `await_ready()` still
    // returns false, so `co_await`ing it dereferences null inside the
    // *awaiting* coroutine with this mock named nowhere in the backtrace
    // (task 698; CLAUDE.md, "Unit Test Guidance").
    //
    // `Cancel` rather than the value-initialised `Ok`: a mock nobody stubbed
    // has not confirmed anything, which is the same reasoning that makes
    // `Bad_NotSupported` the default for a status-returning method.
    ON_CALL(*this, RunMessageBox(/*message=*/_, /*title=*/_, /*mode=*/_))
        .WillByDefault([](std::u16string_view, std::u16string_view,
                          MessageBoxMode) -> Awaitable<MessageBoxResult> {
          co_return MessageBoxResult::Cancel;
        });

    // An empty path is what the Qt implementation returns when the operator
    // dismisses the dialog, so a caller that handles cancellation already
    // handles this.
    ON_CALL(*this, SelectSaveFile(/*params=*/_))
        .WillByDefault([](const SaveParams&)
                           -> Awaitable<std::filesystem::path> {
          co_return std::filesystem::path{};
        });
  }

  MOCK_METHOD(UiView*, GetDialogOwningWindow, (), (const override));

  MOCK_METHOD(UiView*, GetParentWidget, (), (const override));

  MOCK_METHOD(Awaitable<MessageBoxResult>,
              RunMessageBox,
              (std::u16string_view message,
               std::u16string_view title,
               MessageBoxMode mode),
              (override));

  MOCK_METHOD(Awaitable<std::filesystem::path>,
              SelectOpenFile,
              (std::u16string_view title),
              (override));

  MOCK_METHOD(Awaitable<std::filesystem::path>,
              SelectSaveFile,
              (const SaveParams& params),
              (override));
};
