#pragma once

#include "base/strings/string_piece.h"

template <class STRING_TYPE>
int HumanCompareTextT(base::BasicStringPiece<STRING_TYPE> left,
                      base::BasicStringPiece<STRING_TYPE> right);

inline int HumanCompareText(base::StringPiece left, base::StringPiece right) {
  return HumanCompareTextT(left, right);
}

inline int HumanCompareText(base::StringPiece16 left, base::StringPiece16 right) {
  return HumanCompareTextT(left, right);
}
