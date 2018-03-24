#pragma once

#include "base/memory/ref_counted.h"
#include "base/strings/string_piece.h"

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
  scoped_refptr<SpeechLib::ISpVoice> voice_;
};
