#pragma once

#include "node_service/node_ref.h"
#include "components/limits/limit_dialog.h"

class TaskManager;

class LimitModel : private LimitDialogContext {
 public:
  explicit LimitModel(LimitDialogContext&& context);

  struct Limits {
    std::wstring lo;
    std::wstring hi;
    std::wstring lolo;
    std::wstring hihi;
  };

  std::wstring GetSourceTitle() const;
  Limits GetLimits() const;

  void WriteLimits(const Limits& limits);
};
