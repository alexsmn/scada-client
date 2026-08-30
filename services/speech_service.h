#pragma once

#include <string_view>

// Spoken announcements. Only the Windows build has a real voice behind this —
// `Speech` is SAPI — so `is_ok()` is false everywhere else and every caller
// must check it before deciding a spoken announcement has been made.
class SpeechService {
 public:
  virtual ~SpeechService() = default;

  // Whether a voice was acquired. False on every non-Windows build.
  virtual bool is_ok() const = 0;

  // Speaks `text`, asynchronously, interrupting whatever is being spoken.
  // Takes the client's own UTF-16 string rather than `std::wstring_view`: the
  // two are interchangeable only where `wchar_t` is 16 bits, which is a
  // Windows fact rather than a portable one.
  virtual void Speak(std::u16string_view text) = 0;
};
