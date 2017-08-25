#pragma once

#include "ui/views/view.h"

class LoginView : public views::View {
 public:
  LoginView();
};

void RunLoginDialog(gfx::NativeView parent);