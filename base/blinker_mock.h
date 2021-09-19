#pragma once

#include "base/blinker.h"

#include <gmock/gmock.h>

class MockBlinkerManager : public BlinkerManager {
 public:
  MOCK_METHOD(bool, GetState, (), (const override));

  MOCK_METHOD(boost::signals2::scoped_connection,
              Subscribe,
              (const BlinkerCallback& callback),
              (override));
};
