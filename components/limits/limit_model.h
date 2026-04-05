#pragma once

#include "components/limits/limit_dialog.h"
#include "node_service/node_ref.h"

class TaskManager;

class LimitModel : private LimitDialogContext {
 public:
  explicit LimitModel(LimitDialogContext&& context);

  struct Limits {
    std::u16string lo;
    std::u16string hi;
    std::u16string lolo;
    std::u16string hihi;
  };

  std::u16string GetSourceTitle() const;
  Limits GetLimits() const;

  void WriteLimits(const Limits& limits);
};
