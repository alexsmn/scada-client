#pragma once

#include "common/node_ref.h"
#include "common/static_types.h"
#include "common_resources.h"
#include "core/configuration_types.h"
#include "core/view_service.h"
#include "net/transport_string.h"
#include "node_combo_box.h"

#include <algorithm>
#include <set>

using std::max;
using std::min;

#include <atlbase.h>

#include <atlapp.h>
#include <atlctrls.h>
#include <atlframe.h>
#include <atlscrl.h>
#include <atlwin.h>

namespace scada {
struct NodeAttributes;
}

class DialogService;
class NodeService;
class TaskManager;

struct RecordEditorContext {
  DialogService& dialog_service_;
  TaskManager& task_manager_;
  NodeService& node_service_;
};

class RecordEditor : protected RecordEditorContext,
                     public ATL::CDialogImpl<RecordEditor>,
                     public WTL::CScrollImpl<RecordEditor> {
 public:
  explicit RecordEditor(unsigned resource_id, RecordEditorContext&& context);

  const scada::NodeId& node_type_id() const { return node_type_id_; }

  void Init(const NodeRef& node);

  void SetModified(BOOL modified = TRUE);

  virtual void ReadControlsData() {}
  virtual void ReadNodeToControls(const NodeRef& node) {}
  virtual void GetModifiedProperties(scada::NodeAttributes& attributes,
                                     scada::NodeProperties& properties,
                                     scada::ReferenceDescriptions& references) {
  }

  void UpdateData();
  BOOL SaveData();
  void DoPaint(HDC dc) {}

  virtual void OnFinalMessage(HWND) override;

  BEGIN_MSG_MAP(RecordEditor)
  CHAIN_MSG_MAP(WTL::CScrollImpl<RecordEditor>)
  MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
  COMMAND_ID_HANDLER(ID_OK, OnOK)
  COMMAND_ID_HANDLER(ID_CANCEL, OnCancel)
  COMMAND_CODE_HANDLER(EN_CHANGE, OnChange)
  COMMAND_CODE_HANDLER(CBN_SELCHANGE, OnChange)
  COMMAND_CODE_HANDLER(CBN_EDITCHANGE, OnChange)
  COMMAND_CODE_HANDLER(BN_CLICKED, OnChange)
  END_MSG_MAP()

  LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

  LRESULT OnOK(WORD /*wNotifyCode*/,
               WORD /*wID*/,
               HWND /*hWndCtl*/,
               BOOL& /*bHandled*/);
  LRESULT OnCancel(WORD /*wNotifyCode*/,
                   WORD /*wID*/,
                   HWND /*hWndCtl*/,
                   BOOL& /*bHandled*/);
  LRESULT OnChange(WORD /*wNotifyCode*/,
                   WORD /*wID*/,
                   HWND /*hWndCtl*/,
                   BOOL& /*bHandled*/);

  UINT IDD;  // shall be initialized in constructor
  // input
  NodeRef node_;
  // data
  BOOL m_modified = FALSE;
  BOOL lock = TRUE;  // OnChange method is locked

  std::function<void()> destroy_handler;

 private:
  scada::NodeId node_type_id_;  // editing table - shall correspond edit dialog
};

namespace record_editors {

class NamedRecordEditor : public RecordEditor {
 public:
  explicit NamedRecordEditor(unsigned resource_id,
                             RecordEditorContext&& context);

 protected:
  BEGIN_MSG_MAP(NamedRecordEditor)
  MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
  CHAIN_MSG_MAP(__super)
  END_MSG_MAP()

  LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

  // RecordEditor overrides.
  virtual void ReadControlsData() override;
  virtual void ReadNodeToControls(const NodeRef& node) override;
  virtual void GetModifiedProperties(
      scada::NodeAttributes& attributes,
      scada::NodeProperties& properties,
      scada::ReferenceDescriptions& references) override;

 private:
  scada::LocalizedText display_name_;
};

class GroupEditor : public RecordEditor {
 public:
  explicit GroupEditor(RecordEditorContext&& context);

 protected:
  BEGIN_MSG_MAP(GroupEditor)
  MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
  CHAIN_MSG_MAP(__super)
  END_MSG_MAP()

  LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

  // RecordEditor overrides.
  virtual void ReadControlsData() override;
  virtual void ReadNodeToControls(const NodeRef& node) override;
  virtual void GetModifiedProperties(
      scada::NodeAttributes& attributes,
      scada::NodeProperties& properties,
      scada::ReferenceDescriptions& references) override;

 private:
  scada::LocalizedText display_name_;
  bool simulate_;
};

class ItemEditor : public RecordEditor {
 public:
  ItemEditor(UINT IDD, RecordEditorContext&& context);

  BEGIN_MSG_MAP(ItemEditor)
  MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
  COMMAND_HANDLER(IDC_CHAN, CBN_SELCHANGE, OnChanChange)
  COMMAND_CODE_HANDLER(CBN_SELCHANGE, OnSelChange)
  COMMAND_HANDLER(IDC_STALE_CHECK, BN_CLICKED, OnStaleCheckClicked)
  COMMAND_HANDLER(IDC_FORMULA_CHECK, BN_CLICKED, OnFormulaCheckClicked)
  CHAIN_MSG_MAP(__super)
  END_MSG_MAP()

  LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

  LRESULT OnSelChange(WORD /*wNotifyCode*/,
                      WORD /*wID*/,
                      HWND /*hWndCtl*/,
                      BOOL& /*bHandled*/);
  LRESULT OnChanChange(WORD /*wNotifyCode*/,
                       WORD /*wID*/,
                       HWND /*hWndCtl*/,
                       BOOL& /*bHandled*/);
  LRESULT OnStaleCheckClicked(WORD /*wNotifyCode*/,
                              WORD /*wID*/,
                              HWND /*hWndCtl*/,
                              BOOL& /*bHandled*/);
  LRESULT OnFormulaCheckClicked(WORD /*wNotifyCode*/,
                                WORD /*wID*/,
                                HWND /*hWndCtl*/,
                                BOOL& /*bHandled*/);

  // RecordEditor
  virtual void ReadControlsData() override;
  virtual void ReadNodeToControls(const NodeRef& node) override;
  virtual void GetModifiedProperties(
      scada::NodeAttributes& attributes,
      scada::NodeProperties& properties,
      scada::ReferenceDescriptions& references) override;

 private:
  bool SelectDevice(const scada::NodeId& device_id);

  void LoadChannel(unsigned channel_no);
  void SaveChannel();
  scada::NodeId GetDeviceId() const;
  std::string GetChannelPath() const;

  void OnFormulaStateChanged(bool is_formula);

  // windows
  CEdit wnd_name;
  CEdit wnd_alias;
  CEdit wnd_sev;
  WTL::CUpDownCtrl wnd_sev_spin;
  WTL::CComboBox wnd_chan;
  NodeComboBox devices_combo_box_;
  ItemComboBox items_combo_box_;
  WTL::CButton wnd_simulate;
  WTL::CButton formula_checkbox_;
  WTL::CEdit formula_edit_;
  NodeComboBox historical_db_combo_box_;
  NodeComboBox simulation_signal_combo_box_;

  // fields
  scada::LocalizedText display_name_;
  std::string alias;
  int sev;
  bool simulate;

  scada::NodeId simulation_signal_id_;

  int stale_period_;

  scada::NodeId historical_db_id_;

  // data
  unsigned channel_no_;  // number of channel being viewed
  bool allow_direct_item_input_;

  std::string channels_[cfg::NUM_CHANNELS];
  std::string control_channel_;
  std::string* channel_;
};

class TsEditor : public ItemEditor {
 public:
  explicit TsEditor(RecordEditorContext&& context);

  virtual void ReadControlsData() override;
  virtual void ReadNodeToControls(const NodeRef& node) override;
  virtual void GetModifiedProperties(
      scada::NodeAttributes& attributes,
      scada::NodeProperties& properties,
      scada::ReferenceDescriptions& references) override;

  BEGIN_MSG_MAP(TsEditor)
  MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
  CHAIN_MSG_MAP(ItemEditor)
  END_MSG_MAP()

  LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

  // windows
  WTL::CButton wnd_inv;
  NodeComboBox ts_formats_combo_box_;
  // data
  bool inversion;
  scada::NodeId format_id_;
};

class TitEditor : public ItemEditor {
 public:
  explicit TitEditor(RecordEditorContext&& context);

  void UpdateConv();

  virtual void ReadControlsData() override;
  virtual void ReadNodeToControls(const NodeRef& node) override;
  virtual void GetModifiedProperties(
      scada::NodeAttributes& attributes,
      scada::NodeProperties& properties,
      scada::ReferenceDescriptions& references) override;

  BEGIN_MSG_MAP(TitEditor)
  MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
  COMMAND_HANDLER(IDC_CONV_NONE, BN_CLICKED, OnConvClicked)
  COMMAND_HANDLER(IDC_CONV_LINE, BN_CLICKED, OnConvClicked)
  CHAIN_MSG_MAP(__super)
  END_MSG_MAP()

  LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

  LRESULT OnConvClicked(WORD /*wNotifyCode*/,
                        WORD /*wID*/,
                        HWND /*hWndCtl*/,
                        BOOL& /*bHandled*/);

  // windows
  CEdit wnd_eu_lo;
  CEdit wnd_eu_hi;
  CEdit wnd_ir_lo;
  CEdit wnd_ir_hi;
  CButton wnd_clamp;
  CEdit wnd_aperture;
  CEdit wnd_deadband;
  CComboBox wnd_fmt;
  CComboBox wnd_units;
  CButton wnd_conv_none;
  CButton wnd_conv_line;
  // data
  double eu_lo;
  double eu_hi;
  double ir_lo;
  double ir_hi;
  scada::Int32 conv;
  int clamp;
  std::string fmt;
  std::string units;
  double aperture = 0.0;
  double deadband = 0.0;
};

class TsFormatEditor : public RecordEditor,
                       public WTL::COwnerDraw<TsFormatEditor> {
 public:
  explicit TsFormatEditor(RecordEditorContext&& context);

  virtual void ReadControlsData() override;
  virtual void ReadNodeToControls(const NodeRef& node) override;
  virtual void GetModifiedProperties(
      scada::NodeAttributes& attributes,
      scada::NodeProperties& properties,
      scada::ReferenceDescriptions& references) override;

  // COwnerDraw overrides
  void DrawItem(LPDRAWITEMSTRUCT dis);

  BEGIN_MSG_MAP(TsFormatEditor)
  MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
  CHAIN_MSG_MAP(RecordEditor)
  CHAIN_MSG_MAP(WTL::COwnerDraw<TsFormatEditor>)
  END_MSG_MAP()

  LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

  // windows
  WTL::CComboBox wnd_lbl_open;
  WTL::CComboBox wnd_lbl_close;
  WTL::CComboBox wnd_clr_open;
  WTL::CComboBox wnd_clr_close;
  // data
  scada::LocalizedText display_name_;
  std::string lbl_open;
  std::string lbl_close;
  BYTE clr_open;
  BYTE clr_close;
};

class LinkEditor : public NamedRecordEditor {
 public:
  LinkEditor(UINT IDD, RecordEditorContext&& context);

  virtual void ReadControlsData() override;
  virtual void ReadNodeToControls(const NodeRef& node) override;
  virtual void GetModifiedProperties(
      scada::NodeAttributes& attributes,
      scada::NodeProperties& properties,
      scada::ReferenceDescriptions& references) override;

  BEGIN_MSG_MAP(LinkEditor)
  COMMAND_ID_HANDLER(IDC_EDIT_TRANSPORT, OnEditTransport)
  CHAIN_MSG_MAP(__super)
  END_MSG_MAP()

 protected:
  const net::TransportString& transport_string() const {
    return transport_string_;
  }

 private:
  void UpdateTransportString();

  LRESULT OnEditTransport(WORD /*wNotifyCode*/,
                          WORD /*wID*/,
                          HWND /*hWndCtl*/,
                          BOOL& /*bHandled*/);

  bool disabled_;
  net::TransportString transport_string_;
};

class Iec60870LinkEditor : public LinkEditor {
 public:
  Iec60870LinkEditor(UINT IDD, RecordEditorContext&& context);

  virtual void ReadControlsData() override;
  virtual void ReadNodeToControls(const NodeRef& node) override;
  virtual void GetModifiedProperties(
      scada::NodeAttributes& attributes,
      scada::NodeProperties& properties,
      scada::ReferenceDescriptions& references) override;

  BEGIN_MSG_MAP(Iec60870LinkEditor)
  MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
  CHAIN_MSG_MAP(__super)
  END_MSG_MAP()

  LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

  // windows
  WTL::CEdit wnd_max_send;
  WTL::CEdit wnd_max_recv;
  WTL::CEdit wnd_con_timeo;
  WTL::CEdit wnd_term_timeo;
  WTL::CEdit t0_edit_;
  WTL::CComboBox wnd_len_dev;
  WTL::CComboBox wnd_len_cot;
  WTL::CComboBox wnd_len_addr;
  WTL::CButton wnd_collect_data;

  // data
  int max_send;
  int max_recv;
  int con_timeo;
  int term_timeo;
  int len_dev;
  int len_cot;
  int len_addr;
  bool collect_data_;
  int send_timeout_;
  int receive_timeout_;
  int idle_timeout_;
  int num_send_repeats_;
  bool crc_protection_;
  bool anonymous_;
  int t0_;
};

class ModbusLinkEditor : public LinkEditor {
 public:
  explicit ModbusLinkEditor(RecordEditorContext&& context);

  // LinkEditor
  virtual void ReadControlsData() override;
  virtual void ReadNodeToControls(const NodeRef& node) override;
  virtual void GetModifiedProperties(
      scada::NodeAttributes& attributes,
      scada::NodeProperties& properties,
      scada::ReferenceDescriptions& references) override;

 private:
  cfg::ModbusEncoding mode_;
};

class Iec60870DeviceEditor : public RecordEditor {
 public:
  explicit Iec60870DeviceEditor(RecordEditorContext&& context);

  virtual void ReadControlsData() override;
  virtual void ReadNodeToControls(const NodeRef& node) override;
  virtual void GetModifiedProperties(
      scada::NodeAttributes& attributes,
      scada::NodeProperties& properties,
      scada::ReferenceDescriptions& references) override;

  void UpdateSync();

  BEGIN_MSG_MAP(Iec60870DeviceEditor)
  MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
  COMMAND_HANDLER(IDC_SYNC, BN_CLICKED, OnSync)
  CHAIN_MSG_MAP(__super)
  END_MSG_MAP()

  LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnSync(WORD /*wNotifyCode*/,
                 WORD /*wID*/,
                 HWND /*hWndCtl*/,
                 BOOL& /*bHandled*/);

  // windows
  CEdit wnd_name;
  CEdit wnd_addr;
  CEdit wnd_link_addr;
  CEdit wnd_inter_per;
  CButton wnd_sync;
  CEdit wnd_sync_per;
  CButton utc_time_checkbox_;
  CEdit wnd_poll[16];
  // data
  scada::LocalizedText display_name_;
  int addr;
  int link_addr;
  bool poll_on_start_;
  int inter_per;
  bool sync;
  int sync_per;
  int poll[16];
  bool disabled_;
  bool utc_time_ = false;
};

class SimulationItemEditor : public RecordEditor {
 public:
  explicit SimulationItemEditor(RecordEditorContext&& context);

 protected:
  BEGIN_MSG_MAP(SimulationItemEditor)
  MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
  CHAIN_MSG_MAP(__super)
  END_MSG_MAP()

  LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

  // RecordEditor overrides.
  virtual void ReadControlsData() override;
  virtual void ReadNodeToControls(const NodeRef& node) override;
  virtual void GetModifiedProperties(
      scada::NodeAttributes& attributes,
      scada::NodeProperties& properties,
      scada::ReferenceDescriptions& references) override;

 private:
  scada::LocalizedText display_name_;
  cfg::SimulationSignalType type_;
  int period_;
  int phase_;
  int update_rate_;
};

class InplaceDialog : public ATL::CDialogImpl<InplaceDialog> {
 public:
  UINT IDD;

  explicit InplaceDialog(RecordEditor& parent_window)
      : parent_window_(parent_window) {}

  BEGIN_MSG_MAP(RecordEditor)
  COMMAND_CODE_HANDLER(EN_CHANGE, OnChange)
  COMMAND_CODE_HANDLER(CBN_SELCHANGE, OnChange)
  COMMAND_CODE_HANDLER(CBN_EDITCHANGE, OnChange)
  COMMAND_CODE_HANDLER(BN_CLICKED, OnChange)
  END_MSG_MAP()

 private:
  LRESULT OnChange(WORD /*wNotifyCode*/,
                   WORD /*wID*/,
                   HWND /*hWndCtl*/,
                   BOOL& /*bHandled*/);

  RecordEditor& parent_window_;
};

class ModbusDeviceEditor : public NamedRecordEditor {
 public:
  explicit ModbusDeviceEditor(RecordEditorContext&& context);

 protected:
  // RecordEditor overrides.
  virtual void ReadControlsData() override;
  virtual void ReadNodeToControls(const NodeRef& node) override;
  virtual void GetModifiedProperties(
      scada::NodeAttributes& attributes,
      scada::NodeProperties& properties,
      scada::ReferenceDescriptions& references) override;

  BEGIN_MSG_MAP(ModbusDeviceEditor)
  MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
  CHAIN_MSG_MAP(__super)
  END_MSG_MAP()

  LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

 private:
  bool disabled_;
  int address_;
  int repeat_count_;
  int response_timeout_;
};

class HistoricalDBEditor : public NamedRecordEditor {
 public:
  explicit HistoricalDBEditor(RecordEditorContext&& context);

 protected:
  // RecordEditor overrides.
  virtual void ReadControlsData() override;
  virtual void ReadNodeToControls(const NodeRef& node) override;
  virtual void GetModifiedProperties(
      scada::NodeAttributes& attributes,
      scada::NodeProperties& properties,
      scada::ReferenceDescriptions& references) override;

  BEGIN_MSG_MAP(HistoricalDBEditor)
  MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
  CHAIN_MSG_MAP(__super)
  END_MSG_MAP()

  LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

 private:
  int depth_;
};

class Iec61850DeviceEditor : public RecordEditor {
 public:
  explicit Iec61850DeviceEditor(RecordEditorContext&& context);

  virtual void ReadControlsData() override;
  virtual void ReadNodeToControls(const NodeRef& node) override;
  virtual void GetModifiedProperties(
      scada::NodeAttributes& attributes,
      scada::NodeProperties& properties,
      scada::ReferenceDescriptions& references) override;

  BEGIN_MSG_MAP(Iec61850DeviceEditor)
  MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
  CHAIN_MSG_MAP(__super)
  END_MSG_MAP()

  LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

  CEdit name_edit_;
  CButton disabled_button_;
  CEdit host_edit_;
  CEdit port_edit_;

  scada::LocalizedText display_name_;
  bool disabled_;
  std::string host_;
  int port_;
};

class Iec61850RCBEditor : public RecordEditor {
 public:
  explicit Iec61850RCBEditor(RecordEditorContext&& context);

  virtual void ReadControlsData() override;
  virtual void ReadNodeToControls(const NodeRef& node) override;
  virtual void GetModifiedProperties(
      scada::NodeAttributes& attributes,
      scada::NodeProperties& properties,
      scada::ReferenceDescriptions& references) override;

  BEGIN_MSG_MAP(Iec61850RCBEditor)
  MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
  CHAIN_MSG_MAP(__super)
  END_MSG_MAP()

  LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

  CEdit name_edit_;
  CEdit reference_edit_;

  scada::LocalizedText display_name_;
  std::string reference_;
};

}  // namespace record_editors
