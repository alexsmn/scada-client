#pragma once

#include "services/speech_service.h"

#include <wrl/client.h>

namespace SpeechLib {
struct ISpVoice;
}

class Speech final : public SpeechService {
 public:
  Speech();
  ~Speech();

  bool is_ok() const override { return !!voice_; }

  void Speak(const std::wstring_view& text) override;

 private:
  Microsoft::WRL::ComPtr<SpeechLib::ISpVoice> voice_;
};
