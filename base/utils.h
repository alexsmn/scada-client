#pragma once

#include "base/strings/string_piece.h"

template <class T>
int HumanCompareTextT(std::basic_string_view<T> left,
                      std::basic_string_view<T> right);

inline int HumanCompareText(std::string_view left, std::string_view right) {
  return HumanCompareTextT(left, right);
}

inline int HumanCompareText(std::u16string_view left,
                            std::u16string_view right) {
  return HumanCompareTextT(left, right);
}
