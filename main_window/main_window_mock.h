#pragma once

#include "main_window/main_window_interface.h"

#include <gmock/gmock.h>

class MockMainWindow : public MainWindowInterface {
 public:
  MOCK_METHOD(OpenedView*, GetActiveView, (), (override));
  MOCK_METHOD(OpenedView*, GetActiveDataView, (), (override));
};
