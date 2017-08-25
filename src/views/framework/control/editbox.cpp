#include "client/views/framework/control/editbox.h"

namespace framework {

EditBox::EditBox()
    : controller_(NULL),
      suppress_notifications_(false) {
}

void EditBox::OnCommand(UINT notification_code) {
  if (suppress_notifications_)
    return;

  if (notification_code == EN_CHANGE) {
    if (controller_)
      controller_->OnEditBoxChanged(*this);
  }
}

void EditBox::SetText(const base::string16& text) {
  suppress_notifications_ = true;
  SetWindowText(text.c_str());
  suppress_notifications_ = false;
}

} // namespace framework
