#pragma once

#include <string_view>

class SpeechService {
 public:
  virtual ~SpeechService() = default;

  virtual bool is_ok() const = 0;

  virtual void Speak(const std::wstring_view& text) = 0;
};
