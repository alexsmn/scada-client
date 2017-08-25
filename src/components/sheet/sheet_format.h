#pragma once

#include "core/SkColor.h"
#include "client/base/pool.h"
#include "client/components/sheet/conditional_format.h"

struct SheetFormatBase {
  SheetFormatBase()
      : align(DT_LEFT),
        transparent(true),
        color(SK_ColorWHITE) {
  }

  bool operator<(const SheetFormatBase& other) const {
    if (transparent != other.transparent)
      return transparent < other.transparent;
    if (color != other.color)
      return color < other.color;
    if (align != other.align)
      return align < other.align;
    if (conditional_format_ != other.conditional_format_)
      return conditional_format_ < other.conditional_format_;
    return false;
  }

  bool transparent;
  SkColor color;
  unsigned char align;

  scoped_refptr<ConditionalFormat> conditional_format_;
};

class SheetFormat : public SheetFormatBase,
                    public PoolItem<SheetFormatBase, SheetFormat> {
 public:
  explicit SheetFormat(const SheetFormatBase& key)
      : SheetFormatBase(key) {
  }
};

typedef Pool<SheetFormatBase, SheetFormat> SheetFormatPool;
