#pragma once

#include "aui/aui_ns_compat.h"

#include "aui/models/status_bar_model.h"

#include <gmock/gmock.h>

namespace scada::aui {

class MockStatusBarModel : public StatusBarModel {
 public:
  MOCK_METHOD(int, GetPaneCount, (), (const override));
  MOCK_METHOD(std::u16string, GetPaneText, (int index), (const override));
  MOCK_METHOD(int, GetPaneSize, (int index), (const override));

  MOCK_METHOD(boost::signals2::scoped_connection,
              SubscribePanesChanged,
              (const PanesChangedCallback& callback),
              (override));
};

}  // namespace aui
