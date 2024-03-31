#pragma once

#include "main_window/main_window_interface.h"

#include <gmock/gmock.h>

class MockMainWindow : public MainWindowInterface {
 public:
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

  MOCK_METHOD(scada::status_promise<OpenedViewInterface*>,
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
