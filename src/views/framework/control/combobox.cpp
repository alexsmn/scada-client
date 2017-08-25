#include "client/views/framework/control/combobox.h"

namespace framework {

void ComboBox::OnCommand(UINT notification_code) {
  if (notification_code == CBN_SELCHANGE) {
    int new_item = GetCurSel();
    if (controller_)
      controller_->OnItemChanged(*this, -1, new_item);
  }
}

} // namespace framework
