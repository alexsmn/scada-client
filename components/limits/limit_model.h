#pragma once

#include "common/node_ref.h"
#include "components/limits/limit_dialog.h"

class TaskManager;

class LimitModel : private LimitDialogContext {
 public:
  explicit LimitModel(LimitDialogContext&& context);

  struct Limits {
    base::string16 lo;
    base::string16 hi;
    base::string16 lolo;
    base::string16 hihi;
  };

  base::string16 GetSourceTitle() const;
  Limits GetLimits() const;

  void WriteLimits(const Limits& limits);
};
