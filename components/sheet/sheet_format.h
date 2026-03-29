#pragma once

#include "base/pool.h"
#include "conditional_format.h"
#include "aui/color.h"

struct SheetFormatBase {
  bool operator<(const SheetFormatBase& other) const {
    if (color != other.color)
      return color < other.color;
    if (align != other.align)
      return align < other.align;
    if (conditional_format_ != other.conditional_format_)
      return conditional_format_ < other.conditional_format_;
    return false;
  }

  aui::Color color = aui::ColorCode::Transparent;
  unsigned char align = DT_LEFT;

  std::shared_ptr<ConditionalFormat> conditional_format_;
};

class SheetFormat : public SheetFormatBase,
                    public PoolItem<SheetFormatBase, SheetFormat> {
 public:
  explicit SheetFormat(const SheetFormatBase& key) : SheetFormatBase(key) {}
};

typedef Pool<SheetFormatBase, SheetFormat> SheetFormatPool;
