#include "components/property_page/views/record_editors.h"

#include "base/string_util.h"
#include "base/strings/sys_string_conversions.h"
#include "base/win/win_util2.h"
#include "common/formula_util.h"
#include "node_service/node_service.h"
#include "components/transport/transport_dialog.h"
#include "controls/color.h"
#include "core/node_management_service.h"
#include "model/data_items_node_ids.h"
#include "model/devices_node_ids.h"
#include "model/history_node_ids.h"
#include "model/node_id_util.h"
#include "net/transport_string.h"
#include "services/task_manager.h"
#include "skia/ext/skia_utils_win.h"
#include "views/client_utils_views.h"

// RecordEditor

RecordEditor::RecordEditor(unsigned resource_id, RecordEditorContext&& context)
    : RecordEditorContext{std::move(context)}, IDD(resource_id) {}

void RecordEditor::Init(const NodeRef& node) {
  node_ = node;
}

void RecordEditor::OnFinalMessage(HWND) {
  if (destroy_handler)
    destroy_handler();
}

void RecordEditor::SetModified(BOOL modified) {
  GetDlgItem(ID_OK).EnableWindow(modified);
  GetDlgItem(ID_CANCEL).EnableWindow(modified);
  m_modified = modified;
}

BOOL RecordEditor::SaveData() {
  if (node_) {
    ReadControlsData();

    scada::NodeAttributes attributes;
    scada::NodeProperties properties;
    scada::ReferenceDescriptions references;
    GetModifiedProperties(attributes, properties, references);

    if (!attributes.empty() || !properties.empty()) {
      task_manager_.PostUpdateTask(node_.node_id(), std::move(attributes),
                                   std::move(properties));
    }

    for (auto& ref : references) {
      auto ref_id = node_.target(ref.reference_type_id).node_id();
      if (ref_id == ref.node_id)
        continue;

      if (!ref_id.is_null()) {
        task_manager_.PostDeleteReference(ref.reference_type_id,
                                          node_.node_id(), ref_id);
      }

      if (!ref.node_id.is_null()) {
        task_manager_.PostAddReference(ref.reference_type_id, node_.node_id(),
                                       ref.node_id);
      }
    }
  }

  SetModified(FALSE);
  return TRUE;
}

void RecordEditor::UpdateData() {
  if (!node_)
    return;

  assert(!lock);
  lock = TRUE;

  ReadNodeToControls(node_);

  SetModified(FALSE);

  lock = FALSE;
}

LRESULT RecordEditor::OnInitDialog(UINT uMsg,
                                   WPARAM wParam,
                                   LPARAM lParam,
                                   BOOL& bHandled) {
  GetSystemSettings();
  RECT rc;
  GetClientRect(&rc);
  SetScrollSize(rc.right - rc.left, rc.bottom - rc.top, FALSE);

  lock = FALSE;
  bHandled = FALSE;
  return 0;
}

LRESULT RecordEditor::OnOK(WORD /*wNotifyCode*/,
                           WORD /*wID*/,
                           HWND /*hWndCtl*/,
                           BOOL& /*bHandled*/) {
  SaveData();
  return 0;
}

LRESULT RecordEditor::OnCancel(WORD /*wNotifyCode*/,
                               WORD /*wID*/,
                               HWND /*hWndCtl*/,
                               BOOL& /*bHandled*/) {
  UpdateData();
  return 0;
}

LRESULT RecordEditor::OnChange(WORD /*wNotifyCode*/,
                               WORD wID,
                               HWND /*hWndCtl*/,
                               BOOL& /*bHandled*/) {
  if (!lock)
    SetModified(TRUE);
  return 0;
}

namespace record_editors {

// NamedRecordEditor

NamedRecordEditor::NamedRecordEditor(unsigned resource_id,
                                     RecordEditorContext&& context)
    : RecordEditor(resource_id, std::move(context)) {}

LRESULT NamedRecordEditor::OnInitDialog(UINT uMsg,
                                        WPARAM wParam,
                                        LPARAM lParam,
                                        BOOL& bHandled) {
  bHandled = FALSE;
  return 1;
}

void NamedRecordEditor::ReadControlsData() {
  display_name_ =
      scada::ToLocalizedText(win_util::GetWindowText(GetDlgItem(IDC_NAME)));
}

void NamedRecordEditor::ReadNodeToControls(const NodeRef& node) {
  auto name = ToString16(node_.display_name());
  SetDlgItemText(IDC_NAME, name.c_str());
}

void NamedRecordEditor::GetModifiedProperties(
    scada::NodeAttributes& attributes,
    scada::NodeProperties& properties,
    scada::ReferenceDescriptions& references) {
  attributes.set_display_name(display_name_);
}

// GroupEditor

GroupEditor::GroupEditor(RecordEditorContext&& context)
    : RecordEditor(IDD_GROUP, std::move(context)) {}

LRESULT GroupEditor::OnInitDialog(UINT uMsg,
                                  WPARAM wParam,
                                  LPARAM lParam,
                                  BOOL& bHandled) {
  bHandled = FALSE;
  return 1;
}

void GroupEditor::ReadControlsData() {
  display_name_ =
      scada::ToLocalizedText(win_util::GetWindowText(GetDlgItem(IDC_NAME)));

  simulate_ = CButton(GetDlgItem(IDC_SIMULATE)).GetCheck() == BST_CHECKED;
}

void GroupEditor::ReadNodeToControls(const NodeRef& node) {
  SetDlgItemText(IDC_NAME, ToString16(node.display_name()).c_str());
  bool simulated =
      node[data_items::id::DataGroupType_Simulated].value().get_or(false);
  CButton(GetDlgItem(IDC_SIMULATE))
      .SetCheck(simulated ? BST_CHECKED : BST_UNCHECKED);
}

void GroupEditor::GetModifiedProperties(
    scada::NodeAttributes& attributes,
    scada::NodeProperties& properties,
    scada::ReferenceDescriptions& references) {
  attributes.set_display_name(display_name_);

  properties.emplace_back(data_items::id::DataGroupType_Simulated, simulate_);
}

// ItemEditor

ItemEditor::ItemEditor(UINT IDD, RecordEditorContext&& context)
    : RecordEditor(IDD, std::move(context)),
      channel_no_(0),
      channel_(nullptr),
      allow_direct_item_input_(false) {}

LRESULT ItemEditor::OnInitDialog(UINT uMsg,
                                 WPARAM wParam,
                                 LPARAM lParam,
                                 BOOL& bHandled) {
  wnd_name = GetDlgItem(IDC_NAME);
  wnd_alias = GetDlgItem(IDC_ALIAS);
  wnd_chan = GetDlgItem(IDC_CHAN);
  devices_combo_box_.Init(GetDlgItem(IDC_DEV));
  items_combo_box_.Init(node_service_, GetDlgItem(IDC_ITEM));
  formula_checkbox_ = GetDlgItem(IDC_FORMULA_CHECK);
  formula_edit_ = GetDlgItem(IDC_FORMULA);

  wnd_sev = GetDlgItem(IDC_SEV);
  wnd_sev_spin = GetDlgItem(IDC_SEV_SPIN);
  wnd_sev_spin.SetRange(0, 100);

  wnd_chan.AddString(_T("Основной"));
  wnd_chan.AddString(_T("Резервный"));
  wnd_chan.AddString(_T("Управление"));
  wnd_chan.SetCurSel(0);

  wnd_simulate = GetDlgItem(IDC_SIMULATE);
  simulation_signal_combo_box_.Init(GetDlgItem(IDC_SIMULATION_SIGNAL));

  historical_db_combo_box_.Init(GetDlgItem(IDC_HISTORICAL_DB_COMBO));

  bHandled = FALSE;
  return 1;
}

LRESULT ItemEditor::OnChanChange(WORD /*wNotifyCode*/,
                                 WORD /*wID*/,
                                 HWND /*hWndCtl*/,
                                 BOOL& /*bHandled*/) {
  int nchan = wnd_chan.GetCurSel();
  if (nchan == -1)
    nchan = 0;
  if (nchan != channel_no_) {
    SaveChannel();
    LoadChannel(nchan);
  }
  // bHandled shall be TRUE, so we don't go to RecordEditor::OnChange and set it
  // modified
  return 0;
}

LRESULT ItemEditor::OnSelChange(WORD /*wNotifyCode*/,
                                WORD wID,
                                HWND /*hWndCtl*/,
                                BOOL& bHandled) {
  if (!lock) {
    if (wID == IDC_DEV)
      items_combo_box_.SetDeviceId(GetDeviceId());
  }

  bHandled = FALSE;
  return 0;
}

scada::NodeId ItemEditor::GetDeviceId() const {
  return devices_combo_box_.GetSelection().node_id();
}

std::string ItemEditor::GetChannelPath() const {
  if (formula_checkbox_.GetCheck() == BST_CHECKED) {
    return base::SysWideToNativeMB(win_util::GetWindowText(formula_edit_));

  } else {
    auto node_id = items_combo_box_.GetNodeId();
    return !node_id.is_null() ? MakeNodeIdFormula(node_id) : std::string{};
  }
}

bool ItemEditor::SelectDevice(const scada::NodeId& device_id) {
  auto ok = devices_combo_box_.Select(device_id);
  items_combo_box_.SetDeviceId(device_id);
  return ok;
}

void ItemEditor::LoadChannel(unsigned channel_no) {
  assert(channel_no >= 0 && channel_no < cfg::NUM_CHANNELS + 1);

  channel_no_ = channel_no;

  wnd_chan.SetCurSel(channel_no_);

  channel_ = channel_no_ < cfg::NUM_CHANNELS ? &channels_[channel_no_]
                                             : &control_channel_;
  allow_direct_item_input_ = true;

  scada::NodeId node_id;
  scada::NodeId device_id;
  std::string_view nested_name;
  bool is_formula = !IsNodeIdFormula(*channel_, node_id) ||
                    !IsNestedNodeId(node_id, device_id, nested_name);

  formula_checkbox_.SetCheck(is_formula ? BST_CHECKED : BST_UNCHECKED);
  OnFormulaStateChanged(is_formula);

  SelectDevice(device_id);
  items_combo_box_.SetNodeId(node_id);
  formula_edit_.SetWindowText(
      is_formula ? base::SysNativeMBToWide(*channel_).c_str() : L"");
}

void ItemEditor::SaveChannel() {
  *channel_ = GetChannelPath();
}

void ItemEditor::ReadControlsData() {
  __super::ReadControlsData();

  display_name_ = scada::ToLocalizedText(win_util::GetWindowText(wnd_name));
  alias = base::SysWideToNativeMB(win_util::GetWindowText(wnd_alias));
  // TODO: check and limit name and alias

  historical_db_id_ = historical_db_combo_box_.GetSelection().node_id();

  sev = win_util::GetWindowInt(wnd_sev);

  SaveChannel();

  stale_period_ = 0;
  if (CButton(GetDlgItem(IDC_STALE_CHECK)).GetCheck() == BST_CHECKED)
    stale_period_ = win_util::GetWindowInt(GetDlgItem(IDC_STALE_PERIOD));

  // Simulate.
  simulate = wnd_simulate.GetCheck() == BST_CHECKED;
  simulation_signal_id_ = simulation_signal_combo_box_.GetSelection().node_id();
}

void ItemEditor::ReadNodeToControls(const NodeRef& node) {
  __super::ReadNodeToControls(node);

  wnd_name.SetWindowText(ToString16(node.display_name()).c_str());

  wnd_alias.SetWindowText(
      base::SysNativeMBToWide(
          node[data_items::id::DataItemType_Alias].value().get_or(
              std::string()))
          .c_str());

  win_util::SetWindowTextInt(
      wnd_sev, node[data_items::id::DataItemType_Severity].value().get_or(0));

  auto selected_db_id =
      node.target(history::id::HasHistoricalDatabase).node_id();
  historical_db_combo_box_.Fill(
      node_service_.GetNode(history::id::HistoricalDatabases),
      history::id::HistoricalDatabaseType, selected_db_id);

  channels_[0] =
      node[data_items::id::DataItemType_Input1].value().get_or(std::string());
  channels_[1] =
      node[data_items::id::DataItemType_Input2].value().get_or(std::string());
  control_channel_ =
      node[data_items::id::DataItemType_Output].value().get_or(std::string());

  devices_combo_box_.Fill(node_service_.GetNode(devices::id::Devices),
                          devices::id::DeviceType, {});
  LoadChannel(0);

  auto stale_period =
      node[data_items::id::DataItemType_StalePeriod].value().get_or(0);
  WTL::CButton(GetDlgItem(IDC_STALE_CHECK))
      .SetCheck(stale_period ? BST_CHECKED : BST_UNCHECKED);
  SetDlgItemInt(IDC_STALE_PERIOD, stale_period ? stale_period : 5 * 60, FALSE);
  GetDlgItem(IDC_STALE_PERIOD).EnableWindow(stale_period != 0);

  // Simulation.
  wnd_simulate.SetCheck(
      node[data_items::id::DataItemType_Simulated].value().get_or(false)
          ? BST_CHECKED
          : BST_UNCHECKED);

  const auto& simulation_signal_id =
      node.target(data_items::id::HasSimulationSignal).node_id();
  simulation_signal_combo_box_.Fill(
      node_service_.GetNode(data_items::id::SimulationSignals),
      data_items::id::SimulationSignalType, simulation_signal_id);
}

void ItemEditor::GetModifiedProperties(
    scada::NodeAttributes& attributes,
    scada::NodeProperties& properties,
    scada::ReferenceDescriptions& references) {
  __super::GetModifiedProperties(attributes, properties, references);

  // name and alias - only for single item mode
  if (node_) {
    attributes.set_display_name(display_name_);
    properties.emplace_back(data_items::id::DataItemType_Alias, alias);
  }

  properties.emplace_back(data_items::id::DataItemType_Severity, sev);
  references.push_back(
      {history::id::HasHistoricalDatabase, true, historical_db_id_});

  // channels
  properties.emplace_back(data_items::id::DataItemType_Input1, channels_[0]);
  properties.emplace_back(data_items::id::DataItemType_Input2, channels_[1]);
  properties.emplace_back(data_items::id::DataItemType_Output,
                          control_channel_);

  properties.emplace_back(data_items::id::DataItemType_StalePeriod,
                          stale_period_);

  // Simulation.
  properties.emplace_back(data_items::id::DataItemType_Simulated, simulate);
  references.push_back(
      {data_items::id::HasSimulationSignal, true, simulation_signal_id_});
}

LRESULT ItemEditor::OnStaleCheckClicked(WORD /*wNotifyCode*/,
                                        WORD /*wID*/,
                                        HWND /*hWndCtl*/,
                                        BOOL& bHandled) {
  bool checked = CButton(GetDlgItem(IDC_STALE_CHECK)).GetCheck() == BST_CHECKED;
  WTL::CEdit period_window = GetDlgItem(IDC_STALE_PERIOD);
  period_window.EnableWindow(checked);
  if (checked) {
    period_window.SetFocus();
    period_window.SetSelAll();
  }
  bHandled = FALSE;
  return 0;
}

void ItemEditor::OnFormulaStateChanged(bool is_formula) {
  devices_combo_box_.combo_box().EnableWindow(!is_formula);
  items_combo_box_.combo_box().EnableWindow(!is_formula);
  formula_edit_.EnableWindow(is_formula);
}

LRESULT ItemEditor::OnFormulaCheckClicked(WORD /*wNotifyCode*/,
                                          WORD /*wID*/,
                                          HWND /*hWndCtl*/,
                                          BOOL& bHandled) {
  bool is_formula = formula_checkbox_.GetCheck() == BST_CHECKED;
  OnFormulaStateChanged(is_formula);
  if (is_formula)
    formula_edit_.SetFocus();
  bHandled = FALSE;
  return 0;
}

// TsEditor

TsEditor::TsEditor(RecordEditorContext&& context)
    : ItemEditor(IDD_TS, std::move(context)) {}

void TsEditor::ReadControlsData() {
  __super::ReadControlsData();

  inversion = wnd_inv.GetCheck() == BST_CHECKED;

  format_id_ = ts_formats_combo_box_.GetSelection().node_id();
}

static int GetSelColor(int sel) {
  if (sel >= 0 && static_cast<unsigned>(sel) < aui::GetColorCount())
    return sel;
  else
    return 0;
}

void TsEditor::ReadNodeToControls(const NodeRef& node) {
  __super::ReadNodeToControls(node);

  auto inverted =
      node[data_items::id::DiscreteItemType_Inversion].value().get_or(false);
  wnd_inv.SetCheck(inverted ? BST_CHECKED : BST_UNCHECKED);

  auto ts_format_id = node.target(data_items::id::HasTsFormat).node_id();
  ts_formats_combo_box_.Fill(node_service_.GetNode(data_items::id::TsFormats),
                             data_items::id::TsFormatType, ts_format_id);
}

void TsEditor::GetModifiedProperties(scada::NodeAttributes& attributes,
                                     scada::NodeProperties& properties,
                                     scada::ReferenceDescriptions& references) {
  __super::GetModifiedProperties(attributes, properties, references);

  properties.emplace_back(data_items::id::DiscreteItemType_Inversion,
                          inversion);
  references.push_back({data_items::id::HasTsFormat, true, format_id_});
}

LRESULT TsEditor::OnInitDialog(UINT uMsg,
                               WPARAM wParam,
                               LPARAM lParam,
                               BOOL& bHandled) {
  wnd_inv = GetDlgItem(IDC_INV);
  ts_formats_combo_box_.Init(GetDlgItem(IDC_PARAMS));

  bHandled = FALSE;
  return 0;
}

// TitEditor

TitEditor::TitEditor(RecordEditorContext&& context)
    : ItemEditor(IDD_TIT, std::move(context)) {}

double SaveFloat(CEdit edit) {
  std::wstring str = win_util::GetWindowText(edit);
  return ParseWithDefault(str, 0.0);
}

void LoadFloat(CEdit edit, double val) {
  edit.SetWindowText(WideFormat(val).c_str());
}

void TitEditor::ReadControlsData() {
  __super::ReadControlsData();

  eu_lo = SaveFloat(wnd_eu_lo);
  eu_hi = SaveFloat(wnd_eu_hi);
  ir_lo = SaveFloat(wnd_ir_lo);
  ir_hi = SaveFloat(wnd_ir_hi);
  conv = wnd_conv_line.GetCheck() ? 1 : 0;
  clamp = wnd_clamp.GetCheck();

  aperture = SaveFloat(wnd_aperture);
  deadband = SaveFloat(wnd_deadband);

  fmt = base::SysWideToNativeMB(win_util::GetWindowText(wnd_fmt));
  units = base::SysWideToNativeMB(win_util::GetWindowText(wnd_units));
}

void TitEditor::ReadNodeToControls(const NodeRef& node) {
  __super::ReadNodeToControls(node);

  LoadFloat(wnd_eu_lo,
            node[data_items::id::AnalogItemType_EuLo].value().get_or(0.0));
  LoadFloat(wnd_eu_hi,
            node[data_items::id::AnalogItemType_EuHi].value().get_or(0.0));
  LoadFloat(wnd_ir_lo,
            node[data_items::id::AnalogItemType_IrLo].value().get_or(0.0));
  LoadFloat(wnd_ir_hi,
            node[data_items::id::AnalogItemType_IrHi].value().get_or(0.0));

  auto clamping =
      node[data_items::id::AnalogItemType_Clamping].value().get_or(false);
  wnd_clamp.SetCheck(clamping ? BST_CHECKED : BST_UNCHECKED);

  auto conversion =
      node[data_items::id::AnalogItemType_Conversion].value().get_or(
          static_cast<scada::Int32>(0));
  wnd_conv_none.SetCheck(!conversion);
  wnd_conv_line.SetCheck(conversion);
  UpdateConv();

  LoadFloat(wnd_aperture,
            node[data_items::id::AnalogItemType_Aperture].value().get_or(0.0));
  LoadFloat(wnd_deadband,
            node[data_items::id::AnalogItemType_Deadband].value().get_or(0.0));

  const auto& format =
      node[data_items::id::AnalogItemType_DisplayFormat].value().get_or(
          std::string());
  wnd_fmt.SetWindowText(base::SysNativeMBToWide(format).c_str());
  const auto& units =
      node[data_items::id::AnalogItemType_EngineeringUnits].value().get_or(
          std::string());
  wnd_units.SetWindowText(base::SysNativeMBToWide(units).c_str());
}

void TitEditor::GetModifiedProperties(
    scada::NodeAttributes& attributes,
    scada::NodeProperties& properties,
    scada::ReferenceDescriptions& references) {
  __super::GetModifiedProperties(attributes, properties, references);

  properties.emplace_back(data_items::id::AnalogItemType_EuLo, eu_lo);
  properties.emplace_back(data_items::id::AnalogItemType_EuHi, eu_hi);
  properties.emplace_back(data_items::id::AnalogItemType_IrLo, ir_lo);
  properties.emplace_back(data_items::id::AnalogItemType_IrHi, ir_hi);
  properties.emplace_back(data_items::id::AnalogItemType_Conversion,
                          conv ? 1 : 0);
  properties.emplace_back(data_items::id::AnalogItemType_Clamping, clamp);
  properties.emplace_back(data_items::id::AnalogItemType_DisplayFormat, fmt);
  properties.emplace_back(data_items::id::AnalogItemType_EngineeringUnits,
                          units);
  properties.emplace_back(data_items::id::AnalogItemType_Aperture, aperture);
  properties.emplace_back(data_items::id::AnalogItemType_Deadband, deadband);
}

void TitEditor::UpdateConv() {
  BOOL conv = !wnd_conv_none.GetCheck();
  wnd_ir_lo.EnableWindow(conv);
  wnd_ir_hi.EnableWindow(conv);
}

LRESULT TitEditor::OnInitDialog(UINT uMsg,
                                WPARAM wParam,
                                LPARAM lParam,
                                BOOL& bHandled) {
  wnd_eu_lo = GetDlgItem(IDC_EU_LO);
  wnd_eu_hi = GetDlgItem(IDC_EU_HI);
  wnd_ir_lo = GetDlgItem(IDC_IR_LO);
  wnd_ir_hi = GetDlgItem(IDC_IR_HI);
  wnd_conv_none = GetDlgItem(IDC_CONV_NONE);
  wnd_conv_line = GetDlgItem(IDC_CONV_LINE);
  wnd_clamp = GetDlgItem(IDC_CLAMP);
  wnd_aperture = GetDlgItem(IDC_APERTURE);
  wnd_deadband = GetDlgItem(IDC_DEADBAND);
  wnd_fmt = GetDlgItem(IDC_FMT);
  wnd_units = GetDlgItem(IDC_UNITS);

  bHandled = FALSE;
  return 0;
}

LRESULT TitEditor::OnConvClicked(WORD /*wNotifyCode*/,
                                 WORD /*wID*/,
                                 HWND /*hWndCtl*/,
                                 BOOL& bHandled) {
  UpdateConv();
  bHandled = FALSE;
  return 0;
}

// TsFormatEditor

TsFormatEditor::TsFormatEditor(RecordEditorContext&& context)
    : RecordEditor(IDD_TS_FORMAT, std::move(context)) {}

void TsFormatEditor::ReadControlsData() {
  __super::ReadControlsData();

  display_name_ =
      scada::ToLocalizedText(win_util::GetWindowText(GetDlgItem(IDC_NAME)));
  lbl_open = base::SysWideToNativeMB(win_util::GetWindowText(wnd_lbl_open));
  lbl_close = base::SysWideToNativeMB(win_util::GetWindowText(wnd_lbl_close));
}

void TsFormatEditor::ReadNodeToControls(const NodeRef& node) {
  __super::ReadNodeToControls(node);

  SetDlgItemText(IDC_NAME, ToString16(node.display_name()).c_str());
  wnd_lbl_open.SetWindowText(
      base::SysNativeMBToWide(
          node[data_items::id::TsFormatType_OpenLabel].value().get_or(
              std::string()))
          .c_str());
  wnd_lbl_open.SetWindowText(
      base::SysNativeMBToWide(
          node[data_items::id::TsFormatType_CloseLabel].value().get_or(
              std::string()))
          .c_str());
  wnd_clr_open.SetCurSel(
      node[data_items::id::TsFormatType_OpenColor].value().get_or(0));
  wnd_clr_close.SetCurSel(
      node[data_items::id::TsFormatType_CloseColor].value().get_or(0));
}

void TsFormatEditor::GetModifiedProperties(
    scada::NodeAttributes& attributes,
    scada::NodeProperties& properties,
    scada::ReferenceDescriptions& references) {
  __super::GetModifiedProperties(attributes, properties, references);

  attributes.set_display_name(display_name_);

  auto clr_open = GetSelColor(wnd_clr_open.GetCurSel());
  auto clr_close = GetSelColor(wnd_clr_close.GetCurSel());

  properties.emplace_back(data_items::id::TsFormatType_OpenLabel, lbl_open);
  properties.emplace_back(data_items::id::TsFormatType_CloseLabel, lbl_close);
  properties.emplace_back(data_items::id::TsFormatType_OpenColor, clr_open);
  properties.emplace_back(data_items::id::TsFormatType_CloseColor, clr_close);
}

LRESULT TsFormatEditor::OnInitDialog(UINT uMsg,
                                     WPARAM wParam,
                                     LPARAM lParam,
                                     BOOL& bHandled) {
  wnd_lbl_open = GetDlgItem(IDC_LBL_OPEN);
  wnd_lbl_close = GetDlgItem(IDC_LBL_CLOSE);
  wnd_clr_open = GetDlgItem(IDC_CLR_OPEN);
  wnd_clr_close = GetDlgItem(IDC_CLR_CLOSE);

  for (size_t i = 0; i < aui::GetColorCount(); i++) {
    wnd_clr_close.AddString((LPCTSTR)i);
    wnd_clr_open.AddString((LPCTSTR)i);
  }

  bHandled = FALSE;
  return 0;
}

void TsFormatEditor::DrawItem(LPDRAWITEMSTRUCT dis) {
  assert(dis->CtlID == IDC_CLR_CLOSE || dis->CtlID == IDC_CLR_OPEN);
  CDCHandle dc(dis->hDC);

  BOOL sel = dis->itemState & ODS_SELECTED;
  HBRUSH brush = GetSysColorBrush(sel ? COLOR_HIGHLIGHT : COLOR_WINDOW);

  RECT rect = dis->rcItem;
  dc.FillRect(&rect, brush);

  InflateRect(&rect, -3, -2);
  std::wstring_view text;

  if (dis->itemID >= 0 && dis->itemID < aui::GetColorCount()) {
    int color_index = dis->itemID;
    // draw color
    RECT crect = rect;
    crect.right = crect.left + crect.bottom - crect.top;
    const auto sk_color = aui::GetColor(color_index).sk_color();
    if (SkColorGetA(sk_color) != 0) {
      dc.SelectStockBrush(DC_BRUSH);
      dc.SetDCBrushColor(skia::SkColorToCOLORREF(sk_color));
    } else {
      dc.SelectStockBrush(NULL_BRUSH);
    }
    dc.SelectStockPen(BLACK_PEN);
    dc.Rectangle(&crect);
    // draw text
    text = aui::GetColorName(color_index);
    rect.left = crect.right + 3;
  }

  COLORREF color = GetSysColor(sel ? COLOR_HIGHLIGHTTEXT : COLOR_WINDOWTEXT);
  dc.SetTextColor(color);
  dc.SetBkMode(TRANSPARENT);
  dc.DrawText(text.data(), text.size(), &rect,
              DT_LEFT | DT_VCENTER | DT_SINGLELINE);
}

// LinkEditor

LinkEditor::LinkEditor(UINT IDD, RecordEditorContext&& context)
    : NamedRecordEditor(IDD, std::move(context)) {}

void LinkEditor::UpdateTransportString() {
  SetDlgItemText(IDC_TRANSPORT_EDIT,
                 base::SysNativeMBToWide(transport_string_.ToString()).c_str());
}

void LinkEditor::ReadControlsData() {
  __super::ReadControlsData();

  disabled_ = CButton(GetDlgItem(IDC_DISABLED)).GetCheck() == BST_CHECKED;
}

void LinkEditor::ReadNodeToControls(const NodeRef& node) {
  __super::ReadNodeToControls(node);

  auto disabled = node[devices::id::DeviceType_Disabled].value().get_or(false);
  CButton(GetDlgItem(IDC_DISABLED))
      .SetCheck(disabled ? BST_CHECKED : BST_UNCHECKED);

  transport_string_ = net::TransportString(
      node[devices::id::LinkType_Transport].value().get_or(std::string()));
  UpdateTransportString();
}

void LinkEditor::GetModifiedProperties(
    scada::NodeAttributes& attributes,
    scada::NodeProperties& properties,
    scada::ReferenceDescriptions& references) {
  __super::GetModifiedProperties(attributes, properties, references);

  properties.emplace_back(devices::id::DeviceType_Disabled, disabled_);
  properties.emplace_back(devices::id::LinkType_Transport,
                          transport_string_.ToString());
}

LRESULT LinkEditor::OnEditTransport(WORD /*wNotifyCode*/,
                                    WORD /*wID*/,
                                    HWND /*hWndCtl*/,
                                    BOOL& /*bHandled*/) {
  if (ShowTransportDialog(dialog_service_, transport_string_)) {
    UpdateTransportString();
    SetModified();
  }

  return 0;
}

// Iec60870LinkEditor

Iec60870LinkEditor::Iec60870LinkEditor(UINT IDD, RecordEditorContext&& context)
    : LinkEditor(IDD, std::move(context)) {}

void Iec60870LinkEditor::ReadControlsData() {
  __super::ReadControlsData();

  con_timeo = win_util::GetWindowInt(wnd_con_timeo);
  term_timeo = win_util::GetWindowInt(wnd_term_timeo);
  collect_data_ = wnd_collect_data.GetCheck() == BST_CHECKED;
  anonymous_ = CButton(GetDlgItem(IDC_ANONYMOUS)).GetCheck() == BST_CHECKED;

  len_dev = win_util::GetWindowInt(wnd_len_dev);
  len_cot = win_util::GetWindowInt(wnd_len_cot);
  len_addr = win_util::GetWindowInt(wnd_len_addr);

  if (IDD == IDD_IEC60870_LINK104) {
    max_send = win_util::GetWindowInt(wnd_max_send);
    max_recv = win_util::GetWindowInt(wnd_max_recv);

    send_timeout_ = GetDlgItemInt(IDC_SEND_TIMEOUT);
    receive_timeout_ = GetDlgItemInt(IDC_RECEIVE_TIMEOUT);
    idle_timeout_ = GetDlgItemInt(IDC_IDLE_TIMEOUT);
    t0_ = GetDlgItemInt(IDC_IEC60870_LINK_T0);
    num_send_repeats_ = GetDlgItemInt(IDC_NUM_SEND_REPEATS);
    crc_protection_ =
        CButton(GetDlgItem(IDC_CRC_PROTECTION)).GetCheck() == BST_CHECKED;
  }
}

void SetComboInt(CComboBox combo, int val) {
  std::wstring str = WideFormat(val);
  combo.SelectString(-1, str.c_str());
}

void Iec60870LinkEditor::ReadNodeToControls(const NodeRef& node) {
  __super::ReadNodeToControls(node);

  win_util::SetWindowTextInt(
      wnd_con_timeo,
      node[devices::id::Iec60870LinkType_ConfirmationTimeout].value().get_or(
          0));
  win_util::SetWindowTextInt(
      wnd_term_timeo,
      node[devices::id::Iec60870LinkType_TerminationTimeout].value().get_or(0));
  wnd_collect_data.SetCheck(
      node[devices::id::Iec60870LinkType_DataCollection].value().get_or(false)
          ? BST_CHECKED
          : BST_UNCHECKED);
  CButton(GetDlgItem(IDC_ANONYMOUS))
      .SetCheck(
          node[devices::id::Iec60870LinkType_AnonymousMode].value().get_or(
              false)
              ? BST_CHECKED
              : BST_UNCHECKED);

  SetComboInt(
      wnd_len_dev,
      node[devices::id::Iec60870LinkType_DeviceAddressSize].value().get_or(0));
  SetComboInt(wnd_len_cot,
              node[devices::id::Iec60870LinkType_COTSize].value().get_or(0));
  SetComboInt(
      wnd_len_addr,
      node[devices::id::Iec60870LinkType_InfoAddressSize].value().get_or(0));

  if (IDD == IDD_IEC60870_LINK104) {
    win_util::SetWindowTextInt(
        wnd_max_send,
        node[devices::id::Iec60870LinkType_SendQueueSize].value().get_or(0));
    win_util::SetWindowTextInt(
        wnd_max_recv,
        node[devices::id::Iec60870LinkType_ReceiveQueueSize].value().get_or(0));

    SetDlgItemInt(
        IDC_SEND_TIMEOUT,
        node[devices::id::Iec60870LinkType_SendTimeout].value().get_or(0),
        FALSE);
    SetDlgItemInt(
        IDC_RECEIVE_TIMEOUT,
        node[devices::id::Iec60870LinkType_ReceiveTimeout].value().get_or(0),
        FALSE);
    SetDlgItemInt(
        IDC_IDLE_TIMEOUT,
        node[devices::id::Iec60870LinkType_IdleTimeout].value().get_or(0),
        FALSE);
    SetDlgItemInt(
        IDC_NUM_SEND_REPEATS,
        node[devices::id::Iec60870LinkType_SendRetryCount].value().get_or(0),
        FALSE);
    SetDlgItemInt(
        IDC_IEC60870_LINK_T0,
        node[devices::id::Iec60870LinkType_ConnectTimeout].value().get_or(0),
        FALSE);
    CButton(GetDlgItem(IDC_CRC_PROTECTION))
        .SetCheck(
            node[devices::id::Iec60870LinkType_CRCProtection].value().get_or(
                false)
                ? BST_CHECKED
                : BST_UNCHECKED);
  }
}

void Iec60870LinkEditor::GetModifiedProperties(
    scada::NodeAttributes& attributes,
    scada::NodeProperties& properties,
    scada::ReferenceDescriptions& references) {
  __super::GetModifiedProperties(attributes, properties, references);

  auto mode = transport_string().IsActive() ? cfg::Iec60870Mode::MASTER
                                            : cfg::Iec60870Mode::SLAVE;

  properties.emplace_back(devices::id::Iec60870LinkType_ConfirmationTimeout,
                          con_timeo);
  properties.emplace_back(devices::id::Iec60870LinkType_TerminationTimeout,
                          term_timeo);
  properties.emplace_back(devices::id::Iec60870LinkType_ConnectTimeout, t0_);
  properties.emplace_back(devices::id::Iec60870LinkType_DataCollection,
                          collect_data_);
  properties.emplace_back(devices::id::Iec60870LinkType_AnonymousMode,
                          anonymous_);
  properties.emplace_back(devices::id::Iec60870LinkType_Mode,
                          static_cast<int>(mode));

  properties.emplace_back(devices::id::Iec60870LinkType_DeviceAddressSize,
                          len_dev);
  properties.emplace_back(devices::id::Iec60870LinkType_COTSize, len_cot);
  properties.emplace_back(devices::id::Iec60870LinkType_InfoAddressSize,
                          len_addr);

  if (IDD == IDD_IEC60870_LINK104) {
    properties.emplace_back(devices::id::Iec60870LinkType_SendQueueSize,
                            max_send);
    properties.emplace_back(devices::id::Iec60870LinkType_ReceiveQueueSize,
                            max_recv);
    properties.emplace_back(devices::id::Iec60870LinkType_SendTimeout,
                            send_timeout_);
    properties.emplace_back(devices::id::Iec60870LinkType_ReceiveTimeout,
                            receive_timeout_);
    properties.emplace_back(devices::id::Iec60870LinkType_IdleTimeout,
                            idle_timeout_);
    properties.emplace_back(devices::id::Iec60870LinkType_SendRetryCount,
                            num_send_repeats_);
    properties.emplace_back(devices::id::Iec60870LinkType_CRCProtection,
                            crc_protection_ ? 1 : 0);
  }
}

static void AddComboValues(CComboBox combo, const int* items) {
  for (; *items; ++items) {
    std::wstring str = WideFormat(*items);
    combo.AddString(str.c_str());
  }
}

LRESULT Iec60870LinkEditor::OnInitDialog(UINT uMsg,
                                         WPARAM wParam,
                                         LPARAM lParam,
                                         BOOL& bHandled) {
  wnd_con_timeo = GetDlgItem(IDC_CON_TIMEO);
  wnd_term_timeo = GetDlgItem(IDC_TERM_TIMEO);
  t0_edit_ = GetDlgItem(IDC_IEC60870_LINK_T0);

  wnd_len_dev = GetDlgItem(IDC_LEN_DEV);
  wnd_len_cot = GetDlgItem(IDC_LEN_COT);
  wnd_len_addr = GetDlgItem(IDC_LEN_ADDR);

  wnd_collect_data = GetDlgItem(IDC_COLLECT_DATA);

  static const int lens_dev[] = {1, 2, 0};
  static const int lens_cot[] = {1, 2, 0};
  static const int lens_addr[] = {1, 2, 3, 0};

  AddComboValues(wnd_len_dev, lens_dev);
  AddComboValues(wnd_len_cot, lens_cot);
  AddComboValues(wnd_len_addr, lens_addr);

  if (IDD == IDD_IEC60870_LINK104) {
    wnd_max_send = GetDlgItem(IDC_MAX_SEND);
    wnd_max_recv = GetDlgItem(IDC_MAX_RECV);
  }

  bHandled = FALSE;
  return 0;
}

// Iec60870DeviceEditor

Iec60870DeviceEditor::Iec60870DeviceEditor(RecordEditorContext&& context)
    : RecordEditor(IDD_IEC60870_DEVICE, std::move(context)) {}

void Iec60870DeviceEditor::ReadControlsData() {
  __super::ReadControlsData();

  display_name_ = win_util::GetWindowText(wnd_name);
  poll_on_start_ =
      WTL::CButton(GetDlgItem(IDC_INTER)).GetCheck() == BST_CHECKED;
  inter_per = win_util::GetWindowInt(wnd_inter_per);
  sync = wnd_sync.GetCheck() == BST_CHECKED;
  sync_per = win_util::GetWindowInt(wnd_sync_per);
  utc_time_ = utc_time_checkbox_.GetCheck() == BST_CHECKED;

  addr = win_util::GetWindowInt(wnd_addr);
  link_addr = win_util::GetWindowInt(wnd_link_addr);
  disabled_ = WTL::CButton(GetDlgItem(IDC_DISABLED)).GetCheck() == BST_CHECKED;

  for (int i = 0; i < 16; i++)
    poll[i] = win_util::GetWindowInt(wnd_poll[i]);
}

void Iec60870DeviceEditor::ReadNodeToControls(const NodeRef& node) {
  __super::ReadNodeToControls(node);

  auto parent = node.parent();
  bool is_104 =
      parent && parent[devices::id::Iec60870LinkType_Protocol].value().get_or(
                    0) == static_cast<int>(cfg::Iec60870Protocol::IEC104);
  wnd_link_addr.ShowWindow(is_104 ? SW_HIDE : SW_SHOW);
  GetDlgItem(IDC_LINK_ADDR_LABEL).ShowWindow(is_104 ? SW_HIDE : SW_SHOW);

  wnd_name.SetWindowText(node.display_name().c_str());
  win_util::SetWindowTextInt(
      wnd_addr,
      node[devices::id::Iec60870DeviceType_Address].value().get_or(0));
  WTL::CButton(GetDlgItem(IDC_DISABLED))
      .SetCheck(node[devices::id::DeviceType_Disabled].value().get_or(false)
                    ? BST_CHECKED
                    : BST_UNCHECKED);
  win_util::SetWindowTextInt(
      wnd_link_addr,
      node[devices::id::Iec60870DeviceType_LinkAddress].value().get_or(0));
  WTL::CButton(GetDlgItem(IDC_INTER))
      .SetCheck(node[devices::id::Iec60870DeviceType_StartupInterrogation]
                        .value()
                        .get_or(false)
                    ? BST_CHECKED
                    : BST_UNCHECKED);
  win_util::SetWindowTextInt(
      wnd_inter_per,
      node[devices::id::Iec60870DeviceType_InterrogationPeriod].value().get_or(
          0));
  wnd_sync.SetCheck(
      node[devices::id::Iec60870DeviceType_StartupClockSync].value().get_or(
          false)
          ? BST_CHECKED
          : BST_UNCHECKED);
  win_util::SetWindowTextInt(
      wnd_sync_per,
      node[devices::id::Iec60870DeviceType_ClockSyncPeriod].value().get_or(0));
  utc_time_checkbox_.SetCheck(
      node[devices::id::Iec60870DeviceType_UtcTime].value().get_or(false)
          ? BST_CHECKED
          : BST_UNCHECKED);

  for (int i = 0; i < 16; i++) {
    scada::NodeId prop_id(
        devices::numeric_id::Iec60870DeviceType_InterrogationPeriodGroup1 + i,
        NamespaceIndexes::SCADA);
    win_util::SetWindowTextInt(wnd_poll[i], node[prop_id].value().get_or(0));
  }

  UpdateSync();
}

void Iec60870DeviceEditor::GetModifiedProperties(
    scada::NodeAttributes& attributes,
    scada::NodeProperties& properties,
    scada::ReferenceDescriptions& references) {
  __super::GetModifiedProperties(attributes, properties, references);

  attributes.set_display_name(display_name_);

  properties.emplace_back(devices::id::Iec60870DeviceType_Address, addr);
  properties.emplace_back(devices::id::Iec60870DeviceType_LinkAddress,
                          link_addr);
  properties.emplace_back(devices::id::DeviceType_Disabled, disabled_);
  properties.emplace_back(devices::id::Iec60870DeviceType_StartupInterrogation,
                          poll_on_start_);
  properties.emplace_back(devices::id::Iec60870DeviceType_InterrogationPeriod,
                          inter_per);
  properties.emplace_back(devices::id::Iec60870DeviceType_StartupClockSync,
                          sync);
  properties.emplace_back(devices::id::Iec60870DeviceType_ClockSyncPeriod,
                          sync_per);
  properties.emplace_back(devices::id::Iec60870DeviceType_UtcTime, utc_time_);
  for (int i = 0; i < 16; i++) {
    scada::NodeId prop_id(
        devices::numeric_id::Iec60870DeviceType_InterrogationPeriodGroup1 + i,
        NamespaceIndexes::SCADA);
    properties.emplace_back(prop_id, poll[i]);
  }
}

LRESULT Iec60870DeviceEditor::OnInitDialog(UINT uMsg,
                                           WPARAM wParam,
                                           LPARAM lParam,
                                           BOOL& bHandled) {
  wnd_name = GetDlgItem(IDC_NAME);
  wnd_addr = GetDlgItem(IDC_ADDR);
  wnd_link_addr = GetDlgItem(IDC_LINK_ADDR);
  wnd_inter_per = GetDlgItem(IDC_INTER_PER);
  wnd_sync = GetDlgItem(IDC_SYNC);
  wnd_sync_per = GetDlgItem(IDC_SYNC_PER);
  utc_time_checkbox_ = GetDlgItem(IDC_UTC_TIME);
  for (int i = 0; i < 16; i++)
    wnd_poll[i] = GetDlgItem(IDC_POLL1 + i);

  bHandled = FALSE;
  return 0;
}

void Iec60870DeviceEditor::UpdateSync() {
  BOOL enable = wnd_sync.GetCheck() == BST_CHECKED;
  wnd_sync_per.EnableWindow(enable);
}

LRESULT Iec60870DeviceEditor::OnSync(WORD /*wNotifyCode*/,
                                     WORD /*wID*/,
                                     HWND /*hWndCtl*/,
                                     BOOL& bHandled) {
  UpdateSync();
  bHandled = FALSE;  // allow to update modified state
  return 0;
}

// ModbusLinkEditor

ModbusLinkEditor::ModbusLinkEditor(RecordEditorContext&& context)
    : LinkEditor(IDD_LINK_MODBUS, std::move(context)),
      mode_(cfg::ModbusEncoding::RTU) {}

void ModbusLinkEditor::ReadControlsData() {
  __super::ReadControlsData();

  WTL::CButton ascii_radio = GetDlgItem(IDC_MODBUS_ASCII);
  WTL::CButton tcp_radio = GetDlgItem(IDC_MODBUS_TCP);

  if (ascii_radio.GetCheck() == BST_CHECKED)
    mode_ = cfg::ModbusEncoding::ASCII;
  else if (tcp_radio.GetCheck() == BST_CHECKED)
    mode_ = cfg::ModbusEncoding::TCP;
  else
    mode_ = cfg::ModbusEncoding::RTU;
}

void ModbusLinkEditor::ReadNodeToControls(const NodeRef& node) {
  __super::ReadNodeToControls(node);

  WTL::CButton rtu_radio = GetDlgItem(IDC_MODBUS_RTU);
  WTL::CButton ascii_radio = GetDlgItem(IDC_MODBUS_ASCII);
  WTL::CButton tcp_radio = GetDlgItem(IDC_MODBUS_TCP);

  switch (node[devices::id::ModbusLinkType_Protocol].value().get_or(0)) {
    case cfg::ModbusEncoding::ASCII:
      ascii_radio.SetCheck(BST_CHECKED);
      break;
    case cfg::ModbusEncoding::TCP:
      tcp_radio.SetCheck(BST_CHECKED);
      break;
    default:
      rtu_radio.SetCheck(BST_CHECKED);
      break;
  }
}

void ModbusLinkEditor::GetModifiedProperties(
    scada::NodeAttributes& attributes,
    scada::NodeProperties& properties,
    scada::ReferenceDescriptions& references) {
  __super::GetModifiedProperties(attributes, properties, references);

  properties.emplace_back(devices::id::ModbusLinkType_Protocol,
                          static_cast<int>(mode_));
}

// SimulationItemEditor

SimulationItemEditor::SimulationItemEditor(RecordEditorContext&& context)
    : RecordEditor(IDD_SIMULATION_ITEM, std::move(context)) {}

LRESULT SimulationItemEditor::OnInitDialog(UINT uMsg,
                                           WPARAM wParam,
                                           LPARAM lParam,
                                           BOOL& bHandled) {
  bHandled = FALSE;
  return 1;
}

void SimulationItemEditor::ReadControlsData() {
  display_name_ =
      scada::ToLocalizedText(win_util::GetWindowText(GetDlgItem(IDC_NAME)));

  if (CButton(GetDlgItem(IDC_RAMP)).GetCheck() == BST_CHECKED)
    type_ = cfg::SimulationSignalType::RAMP;
  else if (CButton(GetDlgItem(IDC_STEP)).GetCheck() == BST_CHECKED)
    type_ = cfg::SimulationSignalType::STEP;
  else if (CButton(GetDlgItem(IDC_SIN)).GetCheck() == BST_CHECKED)
    type_ = cfg::SimulationSignalType::SIN;
  else if (CButton(GetDlgItem(IDC_COS)).GetCheck() == BST_CHECKED)
    type_ = cfg::SimulationSignalType::COS;
  else
    type_ = cfg::SimulationSignalType::RANDOM;

  period_ = win_util::GetWindowInt(GetDlgItem(IDC_PERIOD));
  phase_ = win_util::GetWindowInt(GetDlgItem(IDC_PHASE));
  update_rate_ = win_util::GetWindowInt(GetDlgItem(IDC_UPDATE_RATE));
}

void SimulationItemEditor::ReadNodeToControls(const NodeRef& node) {
  SetDlgItemText(IDC_NAME, ToString16(node.display_name()).c_str());

  UINT type_id;
  switch (
      node[data_items::id::SimulationSignalType_Function].value().get_or(0)) {
    case cfg::SimulationSignalType::RAMP:
      type_id = IDC_RAMP;
      break;
    case cfg::SimulationSignalType::STEP:
      type_id = IDC_STEP;
      break;
    case cfg::SimulationSignalType::SIN:
      type_id = IDC_SIN;
      break;
    case cfg::SimulationSignalType::COS:
      type_id = IDC_COS;
      break;
    default:
      type_id = IDC_RANDOM;
      break;
  }
  CheckRadioButton(IDC_RANDOM, IDC_COS, type_id);

  win_util::SetWindowTextInt(
      GetDlgItem(IDC_PERIOD),
      node[data_items::id::SimulationSignalType_Period].value().get_or(0));
  win_util::SetWindowTextInt(
      GetDlgItem(IDC_PHASE),
      node[data_items::id::SimulationSignalType_Phase].value().get_or(0));
  win_util::SetWindowTextInt(
      GetDlgItem(IDC_UPDATE_RATE),
      node[data_items::id::SimulationSignalType_UpdateInterval].value().get_or(
          0));
}

void SimulationItemEditor::GetModifiedProperties(
    scada::NodeAttributes& attributes,
    scada::NodeProperties& properties,
    scada::ReferenceDescriptions& references) {
  attributes.set_display_name(display_name_);

  properties.emplace_back(data_items::id::SimulationSignalType_Function,
                          static_cast<int>(type_));
  properties.emplace_back(data_items::id::SimulationSignalType_Period, period_);
  properties.emplace_back(data_items::id::SimulationSignalType_Phase, phase_);
  properties.emplace_back(data_items::id::SimulationSignalType_UpdateInterval,
                          update_rate_);
}

LRESULT InplaceDialog::OnChange(WORD /*wNotifyCode*/,
                                WORD /*wID*/,
                                HWND /*hWndCtl*/,
                                BOOL& bHandled) {
  if (!parent_window_.lock)
    parent_window_.SetModified(TRUE);
  bHandled = FALSE;
  return 0;
}

// ModbusDeviceEditor

ModbusDeviceEditor::ModbusDeviceEditor(RecordEditorContext&& context)
    : NamedRecordEditor(IDD_MODBUS_DEVICE, std::move(context)) {}

LRESULT ModbusDeviceEditor::OnInitDialog(UINT uMsg,
                                         WPARAM wParam,
                                         LPARAM lParam,
                                         BOOL& bHandled) {
  WTL::CUpDownCtrl(GetDlgItem(IDC_ADDRESS_SPIN)).SetRange(0, 255);
  WTL::CUpDownCtrl(GetDlgItem(IDC_REPEAT_COUNT_SPIN)).SetRange(0, 255);

  bHandled = FALSE;
  return 1;
}

void ModbusDeviceEditor::ReadControlsData() {
  __super::ReadControlsData();

  disabled_ = WTL::CButton(GetDlgItem(IDC_DISABLED)).GetCheck() == BST_CHECKED;
  address_ = win_util::GetWindowInt(GetDlgItem(IDC_ADDRESS_EDIT));
  repeat_count_ = win_util::GetWindowInt(GetDlgItem(IDC_REPEAT_COUNT_EDIT));
  response_timeout_ = win_util::GetWindowInt(GetDlgItem(IDC_RESPONSE_TIMEOUT));
}

void ModbusDeviceEditor::ReadNodeToControls(const NodeRef& node) {
  __super::ReadNodeToControls(node);

  auto disabled = node[devices::id::DeviceType_Disabled].value().get_or(false);
  WTL::CButton(GetDlgItem(IDC_DISABLED))
      .SetCheck(disabled ? BST_CHECKED : BST_UNCHECKED);
  SetDlgItemInt(IDC_ADDRESS_EDIT,
                node[devices::id::ModbusDeviceType_Address].value().get_or(0),
                FALSE);
  SetDlgItemInt(
      IDC_REPEAT_COUNT_EDIT,
      node[devices::id::ModbusDeviceType_SendRetryCount].value().get_or(0),
      FALSE);
  SetDlgItemInt(
      IDC_RESPONSE_TIMEOUT,
      node[devices::id::ModbusDeviceType_ResponseTimeout].value().get_or(0),
      FALSE);
}

void ModbusDeviceEditor::GetModifiedProperties(
    scada::NodeAttributes& attributes,
    scada::NodeProperties& properties,
    scada::ReferenceDescriptions& references) {
  __super::GetModifiedProperties(attributes, properties, references);

  properties.emplace_back(devices::id::ModbusDeviceType_Address, address_);
  properties.emplace_back(devices::id::DeviceType_Disabled, disabled_);
  properties.emplace_back(devices::id::ModbusDeviceType_SendRetryCount,
                          repeat_count_);
  properties.emplace_back(devices::id::ModbusDeviceType_ResponseTimeout,
                          response_timeout_);
}

HistoricalDBEditor::HistoricalDBEditor(RecordEditorContext&& context)
    : NamedRecordEditor(IDD_HISTORICAL_DB, std::move(context)) {}

LRESULT HistoricalDBEditor::OnInitDialog(UINT uMsg,
                                         WPARAM wParam,
                                         LPARAM lParam,
                                         BOOL& bHandled) {
  WTL::CUpDownCtrl(GetDlgItem(IDC_DEPTH_SPIN)).SetRange(1, 999);

  bHandled = FALSE;
  return 1;
}

void HistoricalDBEditor::ReadControlsData() {
  __super::ReadControlsData();

  depth_ = win_util::GetWindowInt(GetDlgItem(IDC_DEPTH_EDIT));
}

void HistoricalDBEditor::ReadNodeToControls(const NodeRef& node) {
  __super::ReadNodeToControls(node);

  auto depth =
      node[history::id::HistoricalDatabaseType_Depth].value().get_or(0);
  SetDlgItemInt(IDC_DEPTH_EDIT, depth, FALSE);
}

void HistoricalDBEditor::GetModifiedProperties(
    scada::NodeAttributes& attributes,
    scada::NodeProperties& properties,
    scada::ReferenceDescriptions& references) {
  __super::GetModifiedProperties(attributes, properties, references);

  properties.emplace_back(history::id::HistoricalDatabaseType_Depth, depth_);
}

// Iec61850DeviceEditor

Iec61850DeviceEditor::Iec61850DeviceEditor(RecordEditorContext&& context)
    : RecordEditor(IDD_IEC61850_DEVICE, std::move(context)) {}

void Iec61850DeviceEditor::ReadControlsData() {
  __super::ReadControlsData();

  display_name_ = win_util::GetWindowText(name_edit_);
  disabled_ = disabled_button_.GetCheck() == BST_CHECKED;
  host_ = base::SysWideToNativeMB(win_util::GetWindowText(host_edit_));
  port_ = win_util::GetWindowInt(port_edit_);
}

void Iec61850DeviceEditor::ReadNodeToControls(const NodeRef& node) {
  __super::ReadNodeToControls(node);

  name_edit_.SetWindowText(node.display_name().c_str());
  disabled_button_.SetCheck(
      node[devices::id::DeviceType_Disabled].value().get_or(false)
          ? BST_CHECKED
          : BST_UNCHECKED);
  host_edit_.SetWindowText(
      base::SysNativeMBToWide(
          node[devices::id::Iec61850DeviceType_Host].value().get_or(
              std::string{}))
          .c_str());
  win_util::SetWindowTextInt(
      port_edit_, node[devices::id::Iec61850DeviceType_Port].value().get_or(0));
}

void Iec61850DeviceEditor::GetModifiedProperties(
    scada::NodeAttributes& attributes,
    scada::NodeProperties& properties,
    scada::ReferenceDescriptions& references) {
  __super::GetModifiedProperties(attributes, properties, references);

  attributes.set_display_name(display_name_);

  properties.emplace_back(devices::id::Iec61850DeviceType_Host, host_);
  properties.emplace_back(devices::id::Iec61850DeviceType_Port, port_);
}

LRESULT Iec61850DeviceEditor::OnInitDialog(UINT uMsg,
                                           WPARAM wParam,
                                           LPARAM lParam,
                                           BOOL& bHandled) {
  name_edit_ = WTL::CEdit(GetDlgItem(IDC_NAME));
  disabled_button_ = WTL::CButton(GetDlgItem(IDC_DISABLED));
  host_edit_ = WTL::CEdit(GetDlgItem(IDC_HOST));
  port_edit_ = WTL::CEdit(GetDlgItem(IDC_PORT));

  bHandled = FALSE;
  return 0;
}

// Iec61850RCBEditor

Iec61850RCBEditor::Iec61850RCBEditor(RecordEditorContext&& context)
    : RecordEditor(IDD_IEC61850_RCB, std::move(context)) {}

void Iec61850RCBEditor::ReadControlsData() {
  __super::ReadControlsData();

  display_name_ = win_util::GetWindowText(name_edit_);
  reference_ =
      base::SysWideToNativeMB(win_util::GetWindowText(reference_edit_));
}

void Iec61850RCBEditor::ReadNodeToControls(const NodeRef& node) {
  __super::ReadNodeToControls(node);

  name_edit_.SetWindowText(node.display_name().c_str());
  reference_edit_.SetWindowText(
      base::SysNativeMBToWide(
          node[devices::id::Iec61850ConfigurableObjectType_Reference]
              .value()
              .get_or(std::string{}))
          .c_str());
}

void Iec61850RCBEditor::GetModifiedProperties(
    scada::NodeAttributes& attributes,
    scada::NodeProperties& properties,
    scada::ReferenceDescriptions& references) {
  __super::GetModifiedProperties(attributes, properties, references);

  attributes.set_display_name(display_name_);

  properties.emplace_back(devices::id::Iec61850ConfigurableObjectType_Reference,
                          reference_);
}

LRESULT Iec61850RCBEditor::OnInitDialog(UINT uMsg,
                                        WPARAM wParam,
                                        LPARAM lParam,
                                        BOOL& bHandled) {
  name_edit_ = WTL::CEdit(GetDlgItem(IDC_NAME));
  reference_edit_ = WTL::CEdit(GetDlgItem(IDC_REF));

  bHandled = FALSE;
  return 0;
}

}  // namespace record_editors
