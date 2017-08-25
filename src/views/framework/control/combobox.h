#pragma once

#include "client/views/framework/widget.h"

namespace framework {

class ComboBox;

class ComboBoxController {
 public:
  virtual void OnItemChanged(ComboBox& sender, int old_index, int new_index) = 0;
};

class ComboBox : public Widget,
                 public WTL::CComboBox {
 public:
  ComboBox()
      : controller_(NULL) { }
      
  void SetController(ComboBoxController* controller) { controller_ = controller; }

 protected:
  // Widget
  virtual void OnCommand(UINT notification_code);

 private:
  ComboBoxController* controller_;
};

} // namespace framework
