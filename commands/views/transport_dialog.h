#pragma once

#include "net/transport_string.h"
#include "views/framework/dialog.h"
#include "views/framework/control/combobox.h"
#include "commands/views/inplace_dialog.h"

class TransportDialog : public framework::Dialog,
                        protected framework::ComboBoxController {
 public:
  TransportDialog(const net::TransportString& transport_string);
 
  const net::TransportString& transport_string() const { return transport_string_; }
  
  bool IsTransportActive() const { return transport_string_.IsActive(); }

 protected:
  // framework::Dialog
  virtual void OnInitDialog();
  virtual void OnOK();

  // framework::ComboBoxController
  virtual void OnItemChanged(framework::ComboBox& sender, int old_index,
                             int new_index);
                             
 private:
  enum ConnectionType {
    CONNECTION_TYPE_TCP_CLIENT,
    CONNECTION_TYPE_TCP_SERVER,
    CONNECTION_TYPE_UDP_CLIENT,
    CONNECTION_TYPE_UDP_SERVER,
    CONNECTION_TYPE_SERIAL,
    CONNECTION_TYPE_COUNT
  };

  void SwitchConnectionType(ConnectionType type);

  void LoadControlsData();

  framework::ComboBox type_combobox_;
  InplaceDialog connection_frame_;

  net::TransportString transport_string_;
};
