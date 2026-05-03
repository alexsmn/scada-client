#pragma once

#include "base/pool.h"
#include "aui/color.h"

#include <string>

class ConditionalFormatBase {
 public:
  std::string positive_format;
  aui::Color positive_color;

  std::string negative_format;
  aui::Color negative_color;

  std::string zero_format;
  aui::Color zero_color;

  bool operator<(const ConditionalFormatBase& other) {
    if (positive_format < other.positive_format)
      return true;
    if (other.positive_format < positive_format)
      return false;

    if (positive_color < other.positive_color)
      return true;
    if (other.positive_color < positive_color)
      return false;

    if (negative_format < other.negative_format)
      return true;
    if (other.negative_format < negative_format)
      return false;

    if (negative_color < other.negative_color)
      return true;
    if (other.negative_color < negative_color)
      return false;

    if (zero_format < other.zero_format)
      return true;
    if (other.zero_format < zero_format)
      return false;

    if (zero_color < other.zero_color)
      return true;
    if (other.zero_color < zero_color)
      return false;

    return false;
  }
};

class ConditionalFormat
    : public ConditionalFormatBase,
      public PoolItem<ConditionalFormatBase, ConditionalFormat> {
 public:
  ConditionalFormat(const ConditionalFormatBase& key)
      : ConditionalFormatBase(key) {}
};

typedef Pool<ConditionalFormatBase, ConditionalFormat> ConditionalFormatPool;
