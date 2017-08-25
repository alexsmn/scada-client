#pragma once

#include "client/views/framework/widget.h"

namespace framework {

class Button;

class ButtonController {
 public:
  virtual void OnButtonPressed(Button& sender) = 0;
};

class Button : public Widget,
               public WTL::CButton {
 public:
  Button()
      : controller_(NULL) { }
 
  void SetController(ButtonController* controller) { controller_ = controller; }

  bool IsChecked() const;
 
 protected:
  // Widget
  virtual void OnCommand(UINT notification_code);

 private:
  ButtonController* controller_;
};

} // namespace framework
