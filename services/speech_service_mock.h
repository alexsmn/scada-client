#pragma once

#include "services/speech_service.h"

#include <gmock/gmock.h>

class MockSpeechService : public SpeechService {
 public:
  MOCK_METHOD(bool, is_ok, (), (const override));
  MOCK_METHOD(void, Speak, (const std::wstring_view& text), (override));
};
