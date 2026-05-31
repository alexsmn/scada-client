#pragma once

#include "base/at_exit.h"

class AppInit {
 public:
  AppInit(int argc, char* argv[]);
  ~AppInit();

private:
  base::AtExitManager at_exit_manager_;
};
