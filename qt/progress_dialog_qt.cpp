#include "commands/views/progress_dialog.h"

class ProgressDialogQt : public ProgressDialog  {
 public:
  // ProgressDialog
  virtual void SetProgress(int range, int position) {}
  virtual void SetStatus(const std::wstring& status) {}
  virtual bool IsCancelled() const { return false; }
  virtual void Close() {}
};

std::unique_ptr<ProgressDialog> CreateProgressDialog() {
  return std::unique_ptr<ProgressDialog>(new ProgressDialogQt);
}