#include "client/services/speech.h"

#include <atlbase.h>

#include "client/services/sapi.h"
#include "client/services/profile.h"
#include "core/monitored_item_service.h"

//#import "libid:C866CA3A-32F7-11D2-9602-00C04F8EE628" raw_interfaces_only

typedef enum SPEAKFLAGS {
  // SpVoice flags
  SPF_DEFAULT = 0,
  SPF_ASYNC = (1L << 0),
  SPF_PURGEBEFORESPEAK = (1L << 1),
  SPF_IS_FILENAME = (1L << 2),
  SPF_IS_XML = (1L << 3),
  SPF_IS_NOT_XML = (1L << 4),
  SPF_PERSIST_XML = (1L << 5),

  // Normalizer flags
  SPF_NLP_SPEAK_PUNC = (1L << 6),

  // Masks
  SPF_NLP_MASK = (SPF_NLP_SPEAK_PUNC),
  SPF_VOICE_MASK = (SPF_ASYNC|SPF_PURGEBEFORESPEAK|SPF_IS_FILENAME|
  SPF_IS_XML|SPF_IS_NOT_XML|SPF_NLP_MASK|SPF_PERSIST_XML),
  SPF_UNUSED_FLAGS = ~(SPF_VOICE_MASK)
} SPEAKFLAGS;

Speech::Speech()
    : voice_(NULL) {
  SpeechLib::ISpVoice* voice = nullptr;
  CoCreateInstance(__uuidof(SpeechLib::SpVoice), NULL, CLSCTX_ALL,
      __uuidof(SpeechLib::ISpVoice), (void**)&voice);
  voice_.swap(&voice);
}

Speech::~Speech() {
}

void Speech::Speak(const base::StringPiece16& text) {
  if (!voice_)
    return;

  voice_->Speak(const_cast<LPWSTR>(text.as_string().c_str()),
                SPF_ASYNC | SPF_PURGEBEFORESPEAK, nullptr);
}