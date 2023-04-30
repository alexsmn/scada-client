#pragma once

#include <net/transport_string.h>
#include <string>
#include <vector>

class TransportDialogModel {
 public:
  explicit TransportDialogModel(const net::TransportString& transport_string);

  bool IsSerialPortType(int type_index) const;

  std::vector<std::u16string> type_items;
  int type_index = 0;

  std::u16string network_host;
  int network_port = 0;

  std::vector<std::u16string> serial_port_items;
  int serial_port_index = 0;

  std::vector<std::u16string> baud_rate_items;
  int baud_rate_index = 0;

  std::vector<std::u16string> bit_count_items;
  int bit_count_index = 0;

  std::vector<std::u16string> parity_items;
  int parity_index = 0;

  std::vector<std::u16string> flow_control_items;
  int flow_control_index = 0;

  std::vector<std::u16string> stop_bits_items;
  int stop_bits_index = 0;

  net::TransportString transport_string_;

  void Save();
};
