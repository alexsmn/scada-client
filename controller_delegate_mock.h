#pragma once

#include <gmock/gmock.h>

#include "controller_delegate.h"

class MockControllerDelegate : public ControllerDelegate {
 public:
  MOCK_METHOD(void, SetTitle, (std::u16string_view title), (override));

  MOCK_METHOD(void,
              ShowPopupMenu,
              (unsigned resource_id, const aui::Point& point, bool right_click),
              (override));

  MOCK_METHOD(void, SetModified, (bool modified), (override));

  MOCK_METHOD(void, Close, (), (override));

  MOCK_METHOD(void, OpenView, (const WindowDefinition& def), (override));

  MOCK_METHOD(void,
              ExecuteDefaultNodeCommand,
              (const NodeRef& node),
              (override));

  MOCK_METHOD(ContentsModel*, GetActiveContentsModel, (), (override));

  MOCK_METHOD(void,
              AddContentsObserver,
              (ContentsObserver & observer),
              (override));

  MOCK_METHOD(void,
              RemoveContentsObserver,
              (ContentsObserver & observer),
              (override));

  MOCK_METHOD(void, Focus, (), (override));
};
