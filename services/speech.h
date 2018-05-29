#pragma once

#include "base/strings/string_piece.h"

#include <wrl/client.h>

namespace SpeechLib {
struct ISpVoice;
}

class Speech {
 public:
  Speech();
  ~Speech();

  bool is_ok() const { return !!voice_; }

  void Speak(const base::StringPiece16& text);

 private:
  Microsoft::WRL::ComPtr<SpeechLib::ISpVoice> voice_;
};
