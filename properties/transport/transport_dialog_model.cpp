#include "transport_dialog_model.h"

#include "base/u16format.h"
#include "base/format.h"
#include <boost/algorithm/string/predicate.hpp>
#include "base/utf_convert.h"

#include <algorithm>
#include <format>
#include <transport/transport_string.h>

namespace {

enum ConnectionType {
  CONNECTION_TYPE_TCP_CLIENT,
  CONNECTION_TYPE_TCP_SERVER,
  CONNECTION_TYPE_UDP_CLIENT,
  CONNECTION_TYPE_UDP_SERVER,
  CONNECTION_TYPE_SERIAL,
  CONNECTION_TYPE_COUNT
};

static const std::u16string_view kConnectionTypeStrings[] = {
    u"TCP Client", u"TCP Server", u"UDP Client", u"UDP Server", u"Serial Port"};

static const unsigned kBaudRates[] = {
    75,   110,  134,  150,   300,   600,   1200,  1800,   2400,
    4800, 7200, 9600, 14400, 19200, 38400, 57600, 115200, 128000};
static const std::string_view kStopBitsStrings[] = {"1", "1.5", "2"};
static const int kBitCountFirst = 4;
static const int kBitCountLast = 8;

typedef std::pair<std::u16string_view, std::string_view> StringPair;

static const StringPair kParityStrings[] = {
    StringPair(u"None", "No"), StringPair(u"Even", "Even"),
    StringPair(u"Odd", "Odd"), StringPair(u"Mark", "Mark"),
    StringPair(u"Space", "Space")};

static const StringPair kFlowControlStrings[] = {
    StringPair(u"None", transport::TransportString::kFlowControlNone),
    StringPair(u"XON/XOFF", transport::TransportString::kFlowControlSoftware),
    StringPair(u"Hardware", transport::TransportString::kFlowControlHardware)};

static const char16_t kDefaultString[] = u"<Default>";

static int FindString(const std::string_view strs[],
                      int count,
                      std::string_view value) {
  for (int i = 0; i < count; ++i) {
    if (boost::iequals(strs[i], value)) {
      return i;
    }
  }
  return -1;
}

static int FindStringPair(const StringPair pairs[],
                          int count,
                          std::string_view value) {
  for (int i = 0; i < count; ++i) {
    if (boost::iequals(pairs[i].second, value)) {
      return i;
    }
  }
  return -1;
}

}  // namespace

TransportDialogModel::TransportDialogModel(
    const transport::TransportString& transport_string)
    : transport_string_{transport_string} {
  static_assert(_countof(kConnectionTypeStrings) == CONNECTION_TYPE_COUNT,
                "NotEnoughConnectionTypeStrings");
  for (const auto& str : kConnectionTypeStrings)
    type_items.emplace_back(str);

  transport::TransportString::Protocol protocol = transport_string_.GetProtocol();
  bool active = transport_string_.active();

  ConnectionType connection_type = CONNECTION_TYPE_COUNT;
  switch (protocol) {
    default:
    case transport::TransportString::TCP:
      connection_type =
          active ? CONNECTION_TYPE_TCP_CLIENT : CONNECTION_TYPE_TCP_SERVER;
      break;

    case transport::TransportString::UDP:
      connection_type =
          active ? CONNECTION_TYPE_UDP_CLIENT : CONNECTION_TYPE_UDP_SERVER;
      break;

    case transport::TransportString::SERIAL:
      connection_type = CONNECTION_TYPE_SERIAL;
      break;
  }

  type_index = static_cast<int>(connection_type);

  for (int i = 1; i <= 255; ++i)
    serial_port_items.emplace_back(u16format(L"COM{}:", i));

  baud_rate_items.emplace_back(kDefaultString);
  for (unsigned i = 0; i < std::size(kBaudRates); ++i)
    baud_rate_items.emplace_back(WideFormat(kBaudRates[i]));

  bit_count_items.emplace_back(kDefaultString);
  for (int i = kBitCountFirst; i <= kBitCountLast; ++i)
    bit_count_items.emplace_back(WideFormat(i));

  parity_items.emplace_back(kDefaultString);
  for (unsigned i = 0; i < std::size(kParityStrings); ++i)
    parity_items.emplace_back(kParityStrings[i].first);

  stop_bits_items.emplace_back(kDefaultString);
  for (unsigned i = 0; i < std::size(kStopBitsStrings); ++i) {
    auto sv = kStopBitsStrings[i];
    stop_bits_items.emplace_back(UtfConvert<char16_t>(sv));
  }

  flow_control_items.emplace_back(kDefaultString);
  for (unsigned i = 0; i < std::size(kFlowControlStrings); ++i)
    flow_control_items.emplace_back(kFlowControlStrings[i].first);

  if (connection_type != CONNECTION_TYPE_SERIAL) {
    auto host_sv = transport_string_.GetParamStr(transport::TransportString::kParamHost);
    network_host = UtfConvert<char16_t>(host_sv);
    network_port =
        transport_string_.GetParamInt(transport::TransportString::kParamPort);

  } else {
    const auto name =
        transport_string_.GetParamStr(transport::TransportString::kParamName);
    int port_no = transport::TransportString::ParseSerialPortNumber(name);
    serial_port_index = std::max(0, port_no - 1);

    {
      auto baud_rate = static_cast<unsigned>(
          transport_string_.GetParamInt(transport::TransportString::kParamBaudRate));
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
          transport_string_.GetParamInt(transport::TransportString::kParamByteSize);
      int index = (bit_count >= kBitCountFirst && bit_count <= kBitCountLast)
                      ? bit_count - kBitCountFirst
                      : -1;
      bit_count_index = index + 1;
    }

    {
      const auto parity =
          transport_string_.GetParamStr(transport::TransportString::kParamParity);
      int index =
          FindStringPair(kParityStrings, std::size(kParityStrings), parity);
      parity_index = index + 1;
    }

    {
      const auto stopbits =
          transport_string_.GetParamStr(transport::TransportString::kParamStopBits);
      int index =
          FindString(kStopBitsStrings, std::size(kStopBitsStrings), stopbits);
      stop_bits_index = index + 1;
    }

    {
      const auto flowcontrol = transport_string_.GetParamStr(
          transport::TransportString::kParamFlowControl);
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
  transport_string_ = transport::TransportString();

  ConnectionType type = static_cast<ConnectionType>(type_index);
  switch (type) {
    default:
    case CONNECTION_TYPE_TCP_CLIENT:
    case CONNECTION_TYPE_TCP_SERVER:
      transport_string_.SetProtocol(transport::TransportString::TCP);
      transport_string_.SetActive(type == CONNECTION_TYPE_TCP_CLIENT);
      break;

    case CONNECTION_TYPE_UDP_CLIENT:
    case CONNECTION_TYPE_UDP_SERVER:
      transport_string_.SetProtocol(transport::TransportString::UDP);
      transport_string_.SetActive(type == CONNECTION_TYPE_UDP_CLIENT);
      break;

    case CONNECTION_TYPE_SERIAL: {
      transport_string_.SetProtocol(transport::TransportString::SERIAL);
      break;
    }
  }

  if (type != CONNECTION_TYPE_SERIAL) {
    transport_string_.SetParam(transport::TransportString::kParamHost,
                               UtfConvert<char>(network_host));
    transport_string_.SetParam(transport::TransportString::kParamPort, network_port);

  } else {
    {
      int port_no = serial_port_index + 1;
      if (port_no <= 1)
        port_no = 1;
      transport_string_.SetParam(transport::TransportString::kParamName,
                                 std::format("COM{}", port_no));
    }

    {
      int i = baud_rate_index;
      if (i >= 1 && i <= static_cast<int>(std::size(kBaudRates))) {
        transport_string_.SetParam(transport::TransportString::kParamBaudRate,
                                   kBaudRates[i - 1]);
      } else
        transport_string_.RemoveParam(transport::TransportString::kParamBaudRate);
    }

    {
      int i = bit_count_index;
      if (i >= 1) {
        unsigned bitcount = static_cast<unsigned>(i - 1 + kBitCountFirst);
        transport_string_.SetParam(transport::TransportString::kParamByteSize,
                                   bitcount);
      } else
        transport_string_.RemoveParam(transport::TransportString::kParamByteSize);
    }

    {
      int i = parity_index;
      if (i >= 1) {
        transport_string_.SetParam(transport::TransportString::kParamParity,
                                   kParityStrings[i - 1].second);
      } else
        transport_string_.RemoveParam(transport::TransportString::kParamParity);
    }

    {
      int i = stop_bits_index;
      if (i >= 1) {
        transport_string_.SetParam(transport::TransportString::kParamStopBits,
                                   kStopBitsStrings[i - 1]);
      } else
        transport_string_.RemoveParam(transport::TransportString::kParamStopBits);
    }

    {
      int i = flow_control_index;
      if (i >= 1) {
        transport_string_.SetParam(transport::TransportString::kParamFlowControl,
                                   kFlowControlStrings[i - 1].second);
      } else
        transport_string_.RemoveParam(transport::TransportString::kParamFlowControl);
    }
  }
}
