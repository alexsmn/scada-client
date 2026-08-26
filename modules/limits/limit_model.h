#pragma once

#include "modules/limits/limit_dialog.h"
#include "node_service/node_ref.h"
#include "scada/status.h"

#include <functional>
#include <memory>
#include <string>

class DialogService;
class TaskManager;

// Edits an analog item's four alarm bands and posts them back to the server.
//
// The post is asynchronous, and the model — not the dialog — decides when the
// edit is finished: a write that fails has to be reported to the operator who
// made it, which is impossible once the dialog has closed.
class LimitModel : private LimitDialogContext,
                   public std::enable_shared_from_this<LimitModel> {
 public:
  explicit LimitModel(LimitDialogContext&& context);

  void set_dialog_service(DialogService* dialog_service) {
    dialog_service_ = dialog_service;
  }

  struct Limits {
    std::u16string lo;
    std::u16string hi;
    std::u16string lolo;
    std::u16string hihi;
  };

  std::u16string GetWindowTitle() const;
  std::u16string GetSourceTitle() const;
  Limits GetLimits() const;

  // Posts the operator's edits and returns immediately; the result arrives
  // through `completion_handler` a turn or more later. Re-entrant calls while a
  // post is in flight are ignored, so holding Apply down cannot queue four
  // writes of the same bands.
  void WriteLimits(const Limits& limits);

  // Called with true once the bands are written, and with false when the write
  // failed and the operator has dismissed the error. False leaves the dialog
  // open on the values they typed, so a rejected edit can be corrected or
  // retried rather than silently lost.
  std::function<void(bool ok)> completion_handler;

 private:
  void OnWriteComplete(const scada::Status& status);

  // Owns `message`/`title` for the lifetime of the prompt: the RunMessageBox
  // awaitable is created and awaited inside this coroutine, so a caller's
  // locals would die before the lazy coroutine ever read them.
  static Awaitable<void> ReportWriteErrorAsync(
      std::function<void(bool ok)> completion_handler,
      DialogService& dialog_service,
      std::u16string message,
      std::u16string title);

  DialogService* dialog_service_ = nullptr;
  bool writing_ = false;
};
