#include "base/utils.h"
#include "base/format.h"
#include "base/strings/string_util.h"

#ifdef OS_WIN
#include <windows.h>
#endif

#ifdef OS_WIN
HFONT CreateFont(LPCTSTR name, int height) {
  return ::CreateFont(height, 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET,
                      OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
                      DEFAULT_PITCH, name);
}
#endif

template <class STRING_TYPE>
int ScanEndingNumber(base::BasicStringPiece<STRING_TYPE> str, size_t& len) {
  int count = 0;
  while (len && isdigit(str[len - 1])) {
    --len;
    ++count;
  }
  return ParseWithDefault(str.substr(len, count), -1);
}

template <class STRING_TYPE>
int HumanCompareTextT(base::BasicStringPiece<STRING_TYPE> left,
                      base::BasicStringPiece<STRING_TYPE> right) {
  size_t left_len = left.size();
  size_t right_len = right.size();

  // Skip identical non-digit endings.
  while (left_len && right_len && left[left_len - 1] == right[right_len - 1] &&
         !isdigit(left[left_len - 1])) {
    --left_len;
    --right_len;
  }

  int left_value = ScanEndingNumber(left, left_len);
  int right_value = ScanEndingNumber(right, right_len);

  if (left_len != right_len)
    return base::CompareCaseInsensitiveASCII(left, right);

  int res = base::CompareCaseInsensitiveASCII(left.substr(0, left_len),
                                              right.substr(0, right_len));
  if (res != 0)
    return res;

  return left_value - right_value;
}

template int HumanCompareTextT<std::string>(
    base::BasicStringPiece<std::string> left,
    base::BasicStringPiece<std::string> right);

template int HumanCompareTextT<std::wstring>(
    base::BasicStringPiece<std::wstring> left,
    base::BasicStringPiece<std::wstring> right);
