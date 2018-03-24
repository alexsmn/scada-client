#include "commands/views/transport_dialog.h"

#include "base/strings/sys_string_conversions.h"
#include "base/format.h"
#include "common_resources.h"

#include <algorithm>
#include <atlstr.h>

static const unsigned kBaudRates[] = {
    75, 110, 134, 150, 300, 600, 1200, 1800, 2400,  4800, 7200, 9600, 14400,
    19200, 38400, 57600, 115200, 128000 };
static const RECT kConnectionFrameRect = { 15, 33, 15 + 454, 33 + 102 };
static const base::char16* kConnectionTypeStrings[] = { L"TCP-клиент",
                                                  L"TCP-сервер",
                                                  L"UDP-клиент",
                                                  L"UDP-сервер",
                                                  L"COM-порт" };
static const char* kStopBitsStrings[] = { "1", "1.5", "2" };
static const int kBitCountFirst = 4;
static const int kBitCountLast = 8;

typedef std::pair<const base::char16*, const char*> StringPair;

static const StringPair kParityStrings[] = { StringPair(L"Нет", "No"),
                                             StringPair(L"Чет", "Even"),
                                             StringPair(L"Нечет", "Odd"),
                                             StringPair(L"Маркер", "Mark"),
                                             StringPair(L"Пробел", "Space") };

static const StringPair kFlowControlStrings[] = { StringPair(L"Нет", "No"),
                                                  StringPair(L"XON/XOFF", "XON/XOFF"),
                                                  StringPair(L"Аппаратное", "Hardware") };
                                                  
static const base::char16 kDefaultString[] = L"<Текущее>";

static int FindString(const char* strs[], int count, const char* value) {
  for (int i = 0; i < count; ++i) {
    if (_stricmp(strs[i], value) == 0)
      return i;
  }
  return -1;
}

static int FindStringPair(const StringPair pairs[], int count, const char* value) {
  for (int i = 0; i < count; ++i) {
    if (_stricmp(pairs[i].second, value) == 0)
      return i;
  }
  return -1;
}


TransportDialog::TransportDialog(const net::TransportString& transport_string)
    : framework::Dialog(IDD_TRANSPORT),
      transport_string_(transport_string) {
}

void TransportDialog::OnInitDialog() {
  __super::OnInitDialog();

  type_combobox_.Attach(GetItem(IDC_TYPE_COMBO));
  AttachView(type_combobox_, IDC_TYPE_COMBO);
  type_combobox_.SetController(this);
  static_assert(_countof(kConnectionTypeStrings) == CONNECTION_TYPE_COUNT,
                "NotEnoughConnectionTypeStrings");
  for (int i = 0; i < CONNECTION_TYPE_COUNT; ++i)
    type_combobox_.AddString(kConnectionTypeStrings[i]);

  net::TransportString::Protocol protocol = transport_string_.GetProtocol();
  bool active = transport_string_.IsActive();
  
  ConnectionType type;
  switch (protocol) {
    default:
    case net::TransportString::TCP:
      type = active ? CONNECTION_TYPE_TCP_CLIENT :
                      CONNECTION_TYPE_TCP_SERVER;
      break;

    case net::TransportString::UDP:
      type = active ? CONNECTION_TYPE_UDP_CLIENT :
                      CONNECTION_TYPE_UDP_SERVER;
      break;

    case net::TransportString::SERIAL:
      type = CONNECTION_TYPE_SERIAL;
      break;
  }
  
  type_combobox_.SetCurSel(type);
  SwitchConnectionType(type);
}

void TransportDialog::LoadControlsData() {
  if (connection_frame_.IDD == IDD_CONNECTION_NETWORK) {
    connection_frame_.SetDlgItemText(IDC_HOST_EDIT,
        base::SysNativeMBToWide(transport_string_.GetParamStr(
            net::TransportString::kParamHost)).c_str());
    connection_frame_.SetDlgItemInt(IDC_PORT_EDIT,
        transport_string_.GetParamInt(net::TransportString::kParamPort));
        
  } else {
    WTL::CComboBox port_combo = connection_frame_.GetDlgItem(IDC_PORT_COMBO);
    WTL::CComboBox baudate_combo = connection_frame_.GetDlgItem(IDC_BAUDRATE_COMBO);
    WTL::CComboBox bitcount_combo = connection_frame_.GetDlgItem(IDC_BITCOUNT_COMBO);
    WTL::CComboBox parity_combo = connection_frame_.GetDlgItem(IDC_PARITY_COMBO);
    WTL::CComboBox stopbits_combo = connection_frame_.GetDlgItem(IDC_STOPBITS_COMBO);
    WTL::CComboBox flowcontrol_combo = connection_frame_.GetDlgItem(IDC_FLOWCONTROL_COMBO);
    
    std::string name = transport_string_.GetParamStr(net::TransportString::kParamName);
    int port_no = net::TransportString::ParseSerialPortNumber(name);
    port_combo.SetCurSel(std::max(0, port_no - 1));

    {
      int baudrate = transport_string_.GetParamInt(net::TransportString::kParamBaudRate);
      int index = -1;
      for (int i = 0; i < _countof(kBaudRates); ++i)
        if (kBaudRates[i] == baudrate) {
          index = i;
          break;
        }
      baudate_combo.SetCurSel(index + 1);
    }
    
    {
      int bit_count = transport_string_.GetParamInt(net::TransportString::kParamByteSize);
      int index = (bit_count >= kBitCountFirst && bit_count <= kBitCountLast) ?
          bit_count - kBitCountFirst : -1;
      bitcount_combo.SetCurSel(index + 1);
    }
    
    {
      std::string parity = transport_string_.GetParamStr(net::TransportString::kParamParity);
      int index = FindStringPair(kParityStrings, _countof(kParityStrings), parity.c_str());
      parity_combo.SetCurSel(index + 1);
    }
    
    {
      std::string stopbits = transport_string_.GetParamStr(net::TransportString::kParamStopBits);
      int index = FindString(kStopBitsStrings, _countof(kStopBitsStrings), stopbits.c_str());
      stopbits_combo.SetCurSel(index + 1);
    }

    {
      std::string flowcontrol = transport_string_.GetParamStr(net::TransportString::kParamFlowControl);
      int index = FindStringPair(kFlowControlStrings, _countof(kFlowControlStrings), flowcontrol.c_str());
      flowcontrol_combo.SetCurSel(index + 1);
    }
  }
}

void TransportDialog::OnOK() {
  transport_string_ = net::TransportString();

  ConnectionType type = static_cast<ConnectionType>(
      type_combobox_.GetCurSel());
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

  if (connection_frame_.IDD == IDD_CONNECTION_NETWORK) {
    transport_string_.SetParam(net::TransportString::kParamHost,
        base::SysWideToNativeMB(win_util::GetWindowText(
            connection_frame_.GetDlgItem(IDC_HOST_EDIT))));
    transport_string_.SetParam(net::TransportString::kParamPort,
        base::SysWideToNativeMB(win_util::GetWindowText(
            connection_frame_.GetDlgItem(IDC_PORT_EDIT))));
    
  } else {
    WTL::CComboBox port_combo = connection_frame_.GetDlgItem(IDC_PORT_COMBO);
    WTL::CComboBox baudate_combo = connection_frame_.GetDlgItem(IDC_BAUDRATE_COMBO);
    WTL::CComboBox bitcount_combo = connection_frame_.GetDlgItem(IDC_BITCOUNT_COMBO);
    WTL::CComboBox parity_combo = connection_frame_.GetDlgItem(IDC_PARITY_COMBO);
    WTL::CComboBox stopbits_combo = connection_frame_.GetDlgItem(IDC_STOPBITS_COMBO);
    WTL::CComboBox flowcontrol_combo = connection_frame_.GetDlgItem(IDC_FLOWCONTROL_COMBO);

    {
      int port_no = port_combo.GetCurSel() + 1;
      if (port_no <= 1)
        port_no = 1;
      transport_string_.SetParam(net::TransportString::kParamName,
          base::StringPrintf("COM%d", port_no));
    }

    {
      int i = baudate_combo.GetCurSel();
      if (i >= 1 && i <= _countof(kBaudRates)) {
        transport_string_.SetParam(net::TransportString::kParamBaudRate,
                                   kBaudRates[i - 1]);
      } else
        transport_string_.RemoveParam(net::TransportString::kParamBaudRate);
    }
    
    {
      int i = bitcount_combo.GetCurSel();
      if (i >= 1) {
        unsigned bitcount = static_cast<unsigned>(i - 1 + kBitCountFirst);
        transport_string_.SetParam(net::TransportString::kParamByteSize, bitcount);
      } else
        transport_string_.RemoveParam(net::TransportString::kParamByteSize);
    }
    
    {
      int i = parity_combo.GetCurSel();
      if (i >= 1) {
        transport_string_.SetParam(net::TransportString::kParamParity,
                                   kParityStrings[i - 1].second);
      } else
        transport_string_.RemoveParam(net::TransportString::kParamParity);
    }
    
    {
      int i = stopbits_combo.GetCurSel();
      if (i >= 1) {
        transport_string_.SetParam(net::TransportString::kParamStopBits,
                                   kStopBitsStrings[i - 1]);
      } else 
        transport_string_.RemoveParam(net::TransportString::kParamStopBits);
    }

    {
      int i = flowcontrol_combo.GetCurSel();
      if (i >= 1) {
        transport_string_.SetParam(net::TransportString::kParamFlowControl,
                                   kFlowControlStrings[i - 1].second);
      } else
        transport_string_.RemoveParam(net::TransportString::kParamFlowControl);
    }
  }

  __super::OnOK();
}

void TransportDialog::OnItemChanged(framework::ComboBox& sender,
                                    int old_index, int new_index) {
  int i = type_combobox_.GetCurSel();
  if (i == -1)
    i = 0;
  SwitchConnectionType(static_cast<ConnectionType>(i));
}

void TransportDialog::SwitchConnectionType(ConnectionType type) {
  UINT frame_id = type == CONNECTION_TYPE_SERIAL ? IDD_CONNECTION_SERIAL :
                                                   IDD_CONNECTION_NETWORK;

  if (connection_frame_ && connection_frame_.IDD == frame_id)
    return;

  if (connection_frame_)
    connection_frame_.DestroyWindow();

  connection_frame_.IDD = frame_id;
  connection_frame_.Create(window_handle());
  connection_frame_.SetWindowPos(NULL,
      kConnectionFrameRect.left, kConnectionFrameRect.top,
      kConnectionFrameRect.right - kConnectionFrameRect.left,
      kConnectionFrameRect.bottom - kConnectionFrameRect.top,
      SWP_SHOWWINDOW);
      
  if (frame_id == IDD_CONNECTION_SERIAL) {
    WTL::CComboBox port_combo = connection_frame_.GetDlgItem(IDC_PORT_COMBO);
    for (int i = 1; i <= 255; ++i)
      port_combo.AddString(base::StringPrintf(L"COM%d:", i).c_str());
      
    WTL::CComboBox baudrate_combo = connection_frame_.GetDlgItem(IDC_BAUDRATE_COMBO);
    baudrate_combo.AddString(kDefaultString);
    for (int i = 0; i < _countof(kBaudRates); ++i)
      baudrate_combo.AddString(WideFormat(kBaudRates[i]).c_str());
      
    WTL::CComboBox bitcount_combo = connection_frame_.GetDlgItem(IDC_BITCOUNT_COMBO);
    bitcount_combo.AddString(kDefaultString);
    for (int i = kBitCountFirst; i <= kBitCountLast; ++i)
      bitcount_combo.AddString(WideFormat(i).c_str());

    WTL::CComboBox parity_combo = connection_frame_.GetDlgItem(IDC_PARITY_COMBO);
    parity_combo.AddString(kDefaultString);
    for (int i = 0; i < _countof(kParityStrings); ++i)
      parity_combo.AddString(kParityStrings[i].first);

    WTL::CComboBox stopbits_combo = connection_frame_.GetDlgItem(IDC_STOPBITS_COMBO);
    stopbits_combo.AddString(kDefaultString);
    for (int i = 0; i < _countof(kStopBitsStrings); ++i)
      stopbits_combo.AddString(base::SysNativeMBToWide(kStopBitsStrings[i]).c_str());
      
    WTL::CComboBox flowcontrol_combo = connection_frame_.GetDlgItem(IDC_FLOWCONTROL_COMBO);
    flowcontrol_combo.AddString(kDefaultString);
    for (int i = 0; i < _countof(kFlowControlStrings); ++i)
      flowcontrol_combo.AddString(kFlowControlStrings[i].first);
  }

  LoadControlsData();
}
