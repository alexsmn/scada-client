#pragma once

#include "services/speech_service.h"

#ifdef _WIN32
#include <wrl/client.h>
#endif

namespace SpeechLib {
struct ISpVoice;
}

class Speech final : public SpeechService {
 public:
  Speech();
  ~Speech();

  bool is_ok() const override {
#ifdef _WIN32
    return !!voice_;
#else
    return false;
#endif
  }

  void Speak(const std::wstring_view& text) override;

 private:
#ifdef _WIN32
  Microsoft::WRL::ComPtr<SpeechLib::ISpVoice> voice_;
#endif
};
