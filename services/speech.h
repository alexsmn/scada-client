#pragma once

#include <string_view>
#include <wrl/client.h>

namespace SpeechLib {
struct ISpVoice;
}

class Speech {
 public:
  Speech();
  ~Speech();

  bool is_ok() const { return !!voice_; }

  void Speak(const std::wstring_view& text);

 private:
  Microsoft::WRL::ComPtr<SpeechLib::ISpVoice> voice_;
};
