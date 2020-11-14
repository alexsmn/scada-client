#pragma once

#include <string_view>

template <class T>
int HumanCompareTextT(std::basic_string_view<T> left,
                      std::basic_string_view<T> right);

inline int HumanCompareText(std::string_view left, std::string_view right) {
  return HumanCompareTextT(left, right);
}

inline int HumanCompareText(std::wstring_view left, std::wstring_view right) {
  return HumanCompareTextT(left, right);
}
