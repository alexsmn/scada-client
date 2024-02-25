#pragma once

#include "main_window/main_window_interface.h"

#include <gmock/gmock.h>

class MockMainWindow : public MainWindowInterface {
 public:
  MOCK_METHOD(const Page&, GetCurrentPage, (), (const override));
  MOCK_METHOD(void, OpenPage, (const Page& page), (override));

  MOCK_METHOD(void,
              SetCurrentPageTitle,
              (std::u16string_view title),
              (override));

  MOCK_METHOD(void, SaveCurrentPage, (), (override));
  MOCK_METHOD(void, DeleteCurrentPage, (), (override));

  MOCK_METHOD(OpenedView*, GetActiveView, (), (override));
  MOCK_METHOD(OpenedView*, GetActiveDataView, (), (override));
};
