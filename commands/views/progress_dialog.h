#pragma once

#include <memory>
#include <string>

class ProgressDialog {
 public:
  virtual ~ProgressDialog() {}

  virtual void SetProgress(int range, int position) = 0;
  virtual void SetStatus(const std::wstring& status) = 0;
  virtual bool IsCancelled() const = 0;
};

std::unique_ptr<ProgressDialog> CreateProgressDialog();