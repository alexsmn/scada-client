#include "client/views/framework/control/button.h"

namespace framework {

bool Button::IsChecked() const {
  return GetCheck() == BST_CHECKED;
}

void Button::OnCommand(UINT notification_code) {
  if (!controller_)
    return;

  if (notification_code == BN_CLICKED)
    controller_->OnButtonPressed(*this);
}

} // namespace framework
