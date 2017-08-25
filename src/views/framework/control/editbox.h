#pragma once

#include "base/strings/string16.h"
#include "views/framework/widget.h"

namespace framework {

class EditBox;

class EditBoxController {
 public:
  virtual void OnEditBoxChanged(EditBox& sender) = 0;
};

class EditBox : public Widget,
                public WTL::CEdit {
 public:
  EditBox();

  void SetController(EditBoxController* controller) { controller_ = controller; }

  void SetText(const base::string16& text);

 protected:
  // Widget
  virtual void OnCommand(UINT notification_code);

 private:
  EditBoxController* controller_;

  bool suppress_notifications_;
};

} // namespace framework
