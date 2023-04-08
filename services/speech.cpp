#include "speech.h"

#include "core/monitored_item_service.h"
#include "services/profile.h"

// TODO: Reimport the SAPI headers from the Windows SDK.
#pragma warning(push)
#pragma warning(disable : 4471 4121)
#include "sapi.h"
#pragma warning(pop)

#include <atlbase.h>

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
  SPF_VOICE_MASK =
      (SPF_ASYNC | SPF_PURGEBEFORESPEAK | SPF_IS_FILENAME | SPF_IS_XML |
       SPF_IS_NOT_XML | SPF_NLP_MASK | SPF_PERSIST_XML),
  SPF_UNUSED_FLAGS = ~(SPF_VOICE_MASK)
} SPEAKFLAGS;

Speech::Speech() {
  CoCreateInstance(__uuidof(SpeechLib::SpVoice), nullptr, CLSCTX_ALL,
                   IID_PPV_ARGS(&voice_));
}

Speech::~Speech() {}

void Speech::Speak(const std::wstring_view& text) {
  if (!voice_)
    return;

  voice_->Speak(const_cast<LPWSTR>(std::wstring{text}.c_str()),
                SPF_ASYNC | SPF_PURGEBEFORESPEAK, nullptr);
}
