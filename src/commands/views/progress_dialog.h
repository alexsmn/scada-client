#pragma once

#include <memory>

#include "base/strings/string16.h"

class ProgressDialog {
 public:
  virtual ~ProgressDialog() {}

  virtual void SetProgress(int range, int position) = 0;
  virtual void SetStatus(const base::string16& status) = 0;
  virtual bool IsCancelled() const = 0;
};

std::unique_ptr<ProgressDialog> CreateProgressDialog();