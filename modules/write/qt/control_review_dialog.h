#pragma once

#include "base/awaitable.h"
#include "modules/write/control_confirmation.h"

#include <QDialog>

#include <memory>

class QDialogButtonBox;

// The review an operator answers before a control command is sent (backlog
// 1118, principle §7 in docs/client/ux/principles.md): the object, what it
// reads now, what the command makes it, who the command is logged against,
// when, and which stage of the command this is, then the irreversibility
// warning. It replaced a «Да/Нет» question box.
//
// Laid out by `QDialogButtonBox` (docs/client/ux/dialogs.md §2), its accept
// button names the action — «Send command», never «Yes» (§3) — and Cancel is
// the default, so Return does not send anything (§4).
class ControlReviewDialog : public QDialog {
  Q_OBJECT

 public:
  explicit ControlReviewDialog(const ControlReview& review,
                               QWidget* parent = nullptr);

  // The box holding «Send command» (AcceptRole) and Cancel (RejectRole).
  // Exposed for tests and the screenshot generator.
  QDialogButtonBox* button_box() const { return button_box_; }

 private:
  QDialogButtonBox* button_box_ = nullptr;
};

// Shows `review` modally over `parent` and answers true when the operator
// chose «Send command». Escape, Cancel and the window's close button answer
// false. `review` is by value: the awaitable is lazy.
Awaitable<bool> RunControlReviewDialog(QWidget* parent, ControlReview review);
