#pragma once

#include "components/main/view_manager_delegate.h"

#include <gmock/gmock.h>

class MockViewManagerDelegate : public ViewManagerDelegate {
 public:
  MOCK_METHOD(std::unique_ptr<OpenedView>,
              OnCreateView,
              (WindowDefinition & definition),
              (override));
  MOCK_METHOD(void, OnViewClosed, (OpenedView & view), (override));
  MOCK_METHOD(void, OnActiveViewChanged, (OpenedView * view), (override));
  MOCK_METHOD(void,
              OnShowTabPopupMenu,
              (OpenedView & view, const aui::Point& point),
              (override));
};
