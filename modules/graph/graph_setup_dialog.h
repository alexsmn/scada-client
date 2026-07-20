#pragma once

#include "aui/color.h"

class DialogService;

class GraphSetupDialog {
 public:
  scada::aui::Color color = scada::aui::ColorCode::Black;
  int line_weight_ = 1;
};

bool RunGraphSetupDialog(DialogService& dialog_service,
                         GraphSetupDialog& setup);
