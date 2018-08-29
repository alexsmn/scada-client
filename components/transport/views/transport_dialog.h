#pragma once

#include "commands/views/inplace_dialog.h"
#include "components/transport/transport_dialog_model.h"
#include "net/transport_string.h"
#include "views/framework/control/combobox.h"
#include "views/framework/dialog.h"

class TransportDialog : public framework::Dialog,
                        protected framework::ComboBoxController {
 public:
  explicit TransportDialog(TransportDialogModel& model);

 protected:
  // framework::Dialog
  virtual void OnInitDialog() override;
  virtual void OnOK() override;

  // framework::ComboBoxController
  virtual void OnItemChanged(framework::ComboBox& sender,
                             int old_index,
                             int new_index) override;

 private:
  void SwitchConnectionType(int type_index);

  void LoadControlsData();

  framework::ComboBox type_combobox_;
  InplaceDialog connection_frame_;

  TransportDialogModel& model_;
};
