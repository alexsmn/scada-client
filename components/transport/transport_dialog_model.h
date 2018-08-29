#pragma once

#include "base/strings/string16.h"

namespace net {
class TransportString;
}

class TransportDialogModel {
 public:
  explicit TransportDialogModel(net::TransportString& transport_string);

  bool IsSerialPortType(int type_index) const;

  std::vector<base::string16> type_items;
  int type_index = 0;

  base::string16 network_host;
  int network_port = 0;

  std::vector<base::string16> serial_port_items;
  int serial_port_index = 0;

  std::vector<base::string16> baud_rate_items;
  int baud_rate_index = 0;

  std::vector<base::string16> bit_count_items;
  int bit_count_index = 0;

  std::vector<base::string16> parity_items;
  int parity_index = 0;

  std::vector<base::string16> flow_control_items;
  int flow_control_index = 0;

  std::vector<base::string16> stop_bits_items;
  int stop_bits_index = 0;

  void Save();

 private:
  net::TransportString& transport_string_;
};
