#pragma once

#include "main_window/main_window_interface.h"

#include <gmock/gmock.h>

class MockMainWindow : public MainWindowInterface {
 public:
  MockMainWindow() {
    using namespace testing;

    // A default-constructed `boost::asio::awaitable` -- gmock's fallback for a
    // return type it knows nothing about -- holds a null frame, and its
    // `await_ready()` still reports false, so co_awaiting it segfaults in
    // `await_suspend` inside the *awaiting* coroutine. Hand back an
    // already-complete awaitable instead. Same reason as the defaults in
    // `MockNodeService` and `MockFileManager`.
    ON_CALL(*this, OpenView(_, _))
        .WillByDefault(
            [](const WindowDefinition&,
               bool) -> Awaitable<OpenedViewInterface*> { co_return nullptr; });
  }

  MOCK_METHOD(int, GetMainWindowId, (), (const override));

  // Pages.

  MOCK_METHOD(const Page&, GetCurrentPage, (), (const override));
  MOCK_METHOD(void, OpenPage, (const Page& page), (override));

  MOCK_METHOD(void,
              SetCurrentPageTitle,
              (std::u16string_view title),
              (override));

  MOCK_METHOD(void, SaveCurrentPage, (), (override));
  MOCK_METHOD(void, DeleteCurrentPage, (), (override));

  // Views.

  MOCK_METHOD(OpenedViewInterface*, GetActiveView, (), (const override));
  MOCK_METHOD(OpenedViewInterface*, GetActiveDataView, (), (const override));

  MOCK_METHOD(void,
              ActivateView,
              (const OpenedViewInterface& view),
              (override));

  MOCK_METHOD(std::vector<OpenedViewInterface*>,
              GetOpenedViews,
              (),
              (const override));

  MOCK_METHOD(Awaitable<OpenedViewInterface*>,
              OpenView,
              (const WindowDefinition& window_definition, bool make_active),
              (override));

  MOCK_METHOD(OpenedViewInterface*,
              FindViewByType,
              (std::string_view window_type),
              (const override));

  // Layout.

  MOCK_METHOD(void,
              SplitView,
              (OpenedViewInterface & view, bool vertically),
              (override));
};
