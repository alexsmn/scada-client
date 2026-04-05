#pragma once

#include "aui/models/status_bar_model.h"

#include <gmock/gmock.h>

namespace aui {

class MockStatusBarModel : public StatusBarModel {
 public:
  MOCK_METHOD(int, GetPaneCount, (), (const override));
  MOCK_METHOD(std::u16string, GetPaneText, (int index), (const override));
  MOCK_METHOD(int, GetPaneSize, (int index), (const override));

  MOCK_METHOD(void,
              AddObserver,
              (StatusBarModelObserver & observer),
              (override));

  MOCK_METHOD(void,
              RemoveObserver,
              (StatusBarModelObserver & observer),
              (override));
};

}  // namespace aui
