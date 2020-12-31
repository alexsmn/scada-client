#include "transport_dialog_model.h"

#include "base/string_piece_util.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "base/strings/sys_string_conversions.h"
#include "net/transport_string.h"

#include <algorithm>

namespace {

enum ConnectionType {
  CONNECTION_TYPE_TCP_CLIENT,
  CONNECTION_TYPE_TCP_SERVER,
  CONNECTION_TYPE_UDP_CLIENT,
  CONNECTION_TYPE_UDP_SERVER,
  CONNECTION_TYPE_SERIAL,
  CONNECTION_TYPE_COUNT
};

static const std::wstring_view kConnectionTypeStrings[] = {
    L"TCP-клиент", L"TCP-сервер", L"UDP-клиент", L"UDP-сервер", L"COM-порт"};

static const unsigned kBaudRates[] = {
    75,   110,  134,  150,   300,   600,   1200,  1800,   2400,
    4800, 7200, 9600, 14400, 19200, 38400, 57600, 115200, 128000};
static const std::string_view kStopBitsStrings[] = {"1", "1.5", "2"};
static const int kBitCountFirst = 4;
static const int kBitCountLast = 8;

typedef std::pair<std::wstring_view, std::string_view> StringPair;

static const StringPair kParityStrings[] = {
    StringPair(L"Нет", "No"), StringPair(L"Чет", "Even"),
    StringPair(L"Нечет", "Odd"), StringPair(L"Маркер", "Mark"),
    StringPair(L"Пробел", "Space")};

static const StringPair kFlowControlStrings[] = {
    StringPair(L"Нет", net::TransportString::kFlowControlNone),
    StringPair(L"XON/XOFF", net::TransportString::kFlowControlSoftware),
    StringPair(L"Аппаратное", net::TransportString::kFlowControlHardware)};

static const wchar_t kDefaultString[] = L"<Текущее>";

static int FindString(const std::string_view strs[],
                      int count,
                      std::string_view value) {
  for (int i = 0; i < count; ++i) {
    if (base::EqualsCaseInsensitiveASCII(ToStringPiece(strs[i]),
                                         ToStringPiece(value))) {
      return i;
    }
  }
  return -1;
}

static int FindStringPair(const StringPair pairs[],
                          int count,
                          std::string_view value) {
  for (int i = 0; i < count; ++i) {
    if (base::EqualsCaseInsensitiveASCII(ToStringPiece(pairs[i].second),
                                         ToStringPiece(value))) {
      return i;
    }
  }
  return -1;
}

}  // namespace

TransportDialogModel::TransportDialogModel(
    net::TransportString& transport_string)
    : transport_string_{transport_string} {
  static_assert(_countof(kConnectionTypeStrings) == CONNECTION_TYPE_COUNT,
                "NotEnoughConnectionTypeStrings");
  for (const auto& str : kConnectionTypeStrings)
    type_items.emplace_back(str);

  net::TransportString::Protocol protocol = transport_string_.GetProtocol();
  bool active = transport_string_.IsActive();

  ConnectionType connection_type = CONNECTION_TYPE_COUNT;
  switch (protocol) {
    default:
    case net::TransportString::TCP:
      connection_type =
          active ? CONNECTION_TYPE_TCP_CLIENT : CONNECTION_TYPE_TCP_SERVER;
      break;

    case net::TransportString::UDP:
      connection_type =
          active ? CONNECTION_TYPE_UDP_CLIENT : CONNECTION_TYPE_UDP_SERVER;
      break;

    case net::TransportString::SERIAL:
      connection_type = CONNECTION_TYPE_SERIAL;
      break;
  }

  type_index = static_cast<int>(connection_type);

  for (int i = 1; i <= 255; ++i)
    serial_port_items.emplace_back(base::StringPrintf(L"COM%d:", i));

  baud_rate_items.emplace_back(kDefaultString);
  for (unsigned i = 0; i < std::size(kBaudRates); ++i)
    baud_rate_items.emplace_back(base::NumberToString16(kBaudRates[i]));

  bit_count_items.emplace_back(kDefaultString);
  for (int i = kBitCountFirst; i <= kBitCountLast; ++i)
    bit_count_items.emplace_back(base::NumberToString16(i));

  parity_items.emplace_back(kDefaultString);
  for (unsigned i = 0; i < std::size(kParityStrings); ++i)
    parity_items.emplace_back(kParityStrings[i].first);

  stop_bits_items.emplace_back(kDefaultString);
  for (unsigned i = 0; i < std::size(kStopBitsStrings); ++i) {
    stop_bits_items.emplace_back(
        base::SysNativeMBToWide(ToStringPiece(kStopBitsStrings[i])));
  }

  flow_control_items.emplace_back(kDefaultString);
  for (unsigned i = 0; i < std::size(kFlowControlStrings); ++i)
    flow_control_items.emplace_back(kFlowControlStrings[i].first);

  if (connection_type != CONNECTION_TYPE_SERIAL) {
    network_host = base::SysNativeMBToWide(ToStringPiece(
        transport_string_.GetParamStr(net::TransportString::kParamHost)));
    network_port =
        transport_string_.GetParamInt(net::TransportString::kParamPort);

  } else {
    const auto name =
        transport_string_.GetParamStr(net::TransportString::kParamName);
    int port_no = net::TransportString::ParseSerialPortNumber(name);
    serial_port_index = std::max(0, port_no - 1);

    {
      auto baud_rate = static_cast<unsigned>(
          transport_string_.GetParamInt(net::TransportString::kParamBaudRate));
      int index = -1;
      for (size_t i = 0; i < std::size(kBaudRates); ++i)
        if (kBaudRates[i] == baud_rate) {
          index = static_cast<int>(i);
          break;
        }
      baud_rate_index = index + 1;
    }

    {
      int bit_count =
          transport_string_.GetParamInt(net::TransportString::kParamByteSize);
      int index = (bit_count >= kBitCountFirst && bit_count <= kBitCountLast)
                      ? bit_count - kBitCountFirst
                      : -1;
      bit_count_index = index + 1;
    }

    {
      const auto parity =
          transport_string_.GetParamStr(net::TransportString::kParamParity);
      int index =
          FindStringPair(kParityStrings, std::size(kParityStrings), parity);
      parity_index = index + 1;
    }

    {
      const auto stopbits =
          transport_string_.GetParamStr(net::TransportString::kParamStopBits);
      int index =
          FindString(kStopBitsStrings, std::size(kStopBitsStrings), stopbits);
      stop_bits_index = index + 1;
    }

    {
      const auto flowcontrol = transport_string_.GetParamStr(
          net::TransportString::kParamFlowControl);
      int index = FindStringPair(kFlowControlStrings,
                                 std::size(kFlowControlStrings), flowcontrol);
      flow_control_index = index + 1;
    }
  }
}

bool TransportDialogModel::IsSerialPortType(int type_index) const {
  return type_index == static_cast<int>(CONNECTION_TYPE_SERIAL);
}

void TransportDialogModel::Save() {
  transport_string_ = net::TransportString();

  ConnectionType type = static_cast<ConnectionType>(type_index);
  switch (type) {
    default:
    case CONNECTION_TYPE_TCP_CLIENT:
    case CONNECTION_TYPE_TCP_SERVER:
      transport_string_.SetProtocol(net::TransportString::TCP);
      transport_string_.SetActive(type == CONNECTION_TYPE_TCP_CLIENT);
      break;

    case CONNECTION_TYPE_UDP_CLIENT:
    case CONNECTION_TYPE_UDP_SERVER:
      transport_string_.SetProtocol(net::TransportString::UDP);
      transport_string_.SetActive(type == CONNECTION_TYPE_UDP_CLIENT);
      break;

    case CONNECTION_TYPE_SERIAL: {
      transport_string_.SetProtocol(net::TransportString::SERIAL);
      break;
    }
  }

  if (type != CONNECTION_TYPE_SERIAL) {
    transport_string_.SetParam(net::TransportString::kParamHost,
                               base::SysWideToNativeMB(network_host));
    transport_string_.SetParam(net::TransportString::kParamPort, network_port);

  } else {
    {
      int port_no = serial_port_index + 1;
      if (port_no <= 1)
        port_no = 1;
      transport_string_.SetParam(net::TransportString::kParamName,
                                 base::StringPrintf("COM%d", port_no));
    }

    {
      int i = baud_rate_index;
      if (i >= 1 && i <= static_cast<int>(std::size(kBaudRates))) {
        transport_string_.SetParam(net::TransportString::kParamBaudRate,
                                   kBaudRates[i - 1]);
      } else
        transport_string_.RemoveParam(net::TransportString::kParamBaudRate);
    }

    {
      int i = bit_count_index;
      if (i >= 1) {
        unsigned bitcount = static_cast<unsigned>(i - 1 + kBitCountFirst);
        transport_string_.SetParam(net::TransportString::kParamByteSize,
                                   bitcount);
      } else
        transport_string_.RemoveParam(net::TransportString::kParamByteSize);
    }

    {
      int i = parity_index;
      if (i >= 1) {
        transport_string_.SetParam(net::TransportString::kParamParity,
                                   kParityStrings[i - 1].second);
      } else
        transport_string_.RemoveParam(net::TransportString::kParamParity);
    }

    {
      int i = stop_bits_index;
      if (i >= 1) {
        transport_string_.SetParam(net::TransportString::kParamStopBits,
                                   kStopBitsStrings[i - 1]);
      } else
        transport_string_.RemoveParam(net::TransportString::kParamStopBits);
    }

    {
      int i = flow_control_index;
      if (i >= 1) {
        transport_string_.SetParam(net::TransportString::kParamFlowControl,
                                   kFlowControlStrings[i - 1].second);
      } else
        transport_string_.RemoveParam(net::TransportString::kParamFlowControl);
    }
  }
}
