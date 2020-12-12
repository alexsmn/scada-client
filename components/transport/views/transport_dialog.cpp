#include "components/transport/views/transport_dialog.h"

#include "base/format.h"
#include "base/strings/sys_string_conversions.h"
#include "common_resources.h"
#include "components/transport/transport_dialog.h"
#include "components/transport/transport_dialog_model.h"
#include "services/dialog_service.h"

#include <algorithm>
#include <atlstr.h>

static const RECT kConnectionFrameRect = {15, 33, 15 + 454, 33 + 102};

TransportDialog::TransportDialog(TransportDialogModel& model)
    : framework::Dialog(IDD_TRANSPORT), model_(model) {}

void TransportDialog::OnInitDialog() {
  __super::OnInitDialog();

  type_combobox_.Attach(GetItem(IDC_TYPE_COMBO));
  AttachView(type_combobox_, IDC_TYPE_COMBO);
  type_combobox_.SetController(this);
  for (auto& item : model_.type_items)
    type_combobox_.AddString(item.c_str());

  type_combobox_.SetCurSel(model_.type_index);
  SwitchConnectionType(model_.type_index);
}

void TransportDialog::LoadControlsData() {
  if (connection_frame_.IDD == IDD_CONNECTION_NETWORK) {
    connection_frame_.SetDlgItemText(IDC_HOST_EDIT,
                                     model_.network_host.c_str());
    connection_frame_.SetDlgItemInt(IDC_PORT_EDIT, model_.network_port);

  } else {
    WTL::CComboBox port_combo =
        static_cast<HWND>(connection_frame_.GetDlgItem(IDC_PORT_COMBO));
    WTL::CComboBox baudate_combo =
        static_cast<HWND>(connection_frame_.GetDlgItem(IDC_BAUDRATE_COMBO));
    WTL::CComboBox bitcount_combo =
        static_cast<HWND>(connection_frame_.GetDlgItem(IDC_BITCOUNT_COMBO));
    WTL::CComboBox parity_combo =
        static_cast<HWND>(connection_frame_.GetDlgItem(IDC_PARITY_COMBO));
    WTL::CComboBox stopbits_combo =
        static_cast<HWND>(connection_frame_.GetDlgItem(IDC_STOPBITS_COMBO));
    WTL::CComboBox flowcontrol_combo =
        static_cast<HWND>(connection_frame_.GetDlgItem(IDC_FLOWCONTROL_COMBO));

    port_combo.SetCurSel(model_.serial_port_index);
    baudate_combo.SetCurSel(model_.baud_rate_index);
    bitcount_combo.SetCurSel(model_.bit_count_index);
    parity_combo.SetCurSel(model_.parity_index);
    stopbits_combo.SetCurSel(model_.stop_bits_index);
    flowcontrol_combo.SetCurSel(model_.flow_control_index);
  }
}

void TransportDialog::OnOK() {
  model_.type_index = type_combobox_.GetCurSel();

  if (connection_frame_.IDD == IDD_CONNECTION_NETWORK) {
    model_.network_host =
        win_util::GetWindowText(connection_frame_.GetDlgItem(IDC_HOST_EDIT));
    model_.network_port =
        win_util::GetWindowInt(connection_frame_.GetDlgItem(IDC_PORT_EDIT));

  } else {
    WTL::CComboBox port_combo =
        static_cast<HWND>(connection_frame_.GetDlgItem(IDC_PORT_COMBO));
    WTL::CComboBox baudate_combo =
        static_cast<HWND>(connection_frame_.GetDlgItem(IDC_BAUDRATE_COMBO));
    WTL::CComboBox bitcount_combo =
        static_cast<HWND>(connection_frame_.GetDlgItem(IDC_BITCOUNT_COMBO));
    WTL::CComboBox parity_combo =
        static_cast<HWND>(connection_frame_.GetDlgItem(IDC_PARITY_COMBO));
    WTL::CComboBox stopbits_combo =
        static_cast<HWND>(connection_frame_.GetDlgItem(IDC_STOPBITS_COMBO));
    WTL::CComboBox flowcontrol_combo =
        static_cast<HWND>(connection_frame_.GetDlgItem(IDC_FLOWCONTROL_COMBO));

    model_.serial_port_index = port_combo.GetCurSel();
    model_.baud_rate_index = baudate_combo.GetCurSel();
    model_.bit_count_index = bitcount_combo.GetCurSel();
    model_.parity_index = parity_combo.GetCurSel();
    model_.stop_bits_index = stopbits_combo.GetCurSel();
    model_.flow_control_index = flowcontrol_combo.GetCurSel();
  }

  __super::OnOK();
}

void TransportDialog::OnItemChanged(framework::ComboBox& sender,
                                    int old_index,
                                    int new_index) {
  int i = type_combobox_.GetCurSel();
  if (i == -1)
    i = 0;
  SwitchConnectionType(i);
}

void TransportDialog::SwitchConnectionType(int type_index) {
  UINT frame_id = model_.IsSerialPortType(type_index) ? IDD_CONNECTION_SERIAL
                                                      : IDD_CONNECTION_NETWORK;

  if (connection_frame_ && connection_frame_.IDD == frame_id)
    return;

  if (connection_frame_)
    connection_frame_.DestroyWindow();

  connection_frame_.IDD = frame_id;
  connection_frame_.Create(window_handle());
  connection_frame_.SetWindowPos(
      NULL, kConnectionFrameRect.left, kConnectionFrameRect.top,
      kConnectionFrameRect.right - kConnectionFrameRect.left,
      kConnectionFrameRect.bottom - kConnectionFrameRect.top, SWP_SHOWWINDOW);

  if (frame_id == IDD_CONNECTION_SERIAL) {
    WTL::CComboBox port_combo =
        static_cast<HWND>(connection_frame_.GetDlgItem(IDC_PORT_COMBO));
    for (auto& item : model_.serial_port_items)
      port_combo.AddString(item.c_str());

    WTL::CComboBox baudrate_combo =
        static_cast<HWND>(connection_frame_.GetDlgItem(IDC_BAUDRATE_COMBO));
    for (auto& item : model_.baud_rate_items)
      baudrate_combo.AddString(item.c_str());

    WTL::CComboBox bitcount_combo =
        static_cast<HWND>(connection_frame_.GetDlgItem(IDC_BITCOUNT_COMBO));
    for (auto& item : model_.bit_count_items)
      baudrate_combo.AddString(item.c_str());

    WTL::CComboBox parity_combo =
        static_cast<HWND>(connection_frame_.GetDlgItem(IDC_PARITY_COMBO));
    for (auto& item : model_.parity_items)
      parity_combo.AddString(item.c_str());

    WTL::CComboBox stopbits_combo =
        static_cast<HWND>(connection_frame_.GetDlgItem(IDC_STOPBITS_COMBO));
    for (auto& item : model_.stop_bits_items)
      stopbits_combo.AddString(item.c_str());

    WTL::CComboBox flowcontrol_combo =
        static_cast<HWND>(connection_frame_.GetDlgItem(IDC_FLOWCONTROL_COMBO));
    for (auto& item : model_.flow_control_items)
      stopbits_combo.AddString(item.c_str());
  }

  LoadControlsData();
}

bool ShowTransportDialog(DialogService& dialog_service,
                         net::TransportString& transport_string) {
  TransportDialogModel model{transport_string};
  TransportDialog dialog{model};
  return dialog.Execute(dialog_service.GetDialogOwningWindow()) == IDOK;
}
