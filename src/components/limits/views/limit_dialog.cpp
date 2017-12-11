#include "components/limits/limit_dialog.h"

#include "base/strings/stringprintf.h"
#include "base/strings/sys_string_conversions.h"
#include "views/client_utils_views.h"
#include "services/task_manager.h"
#include "core/data_value.h"
#include "common/scada_node_ids.h"
#include "common_resources.h"
#include "core/configuration_types.h"
#include "dialog_service.h"
#include "common/node_ref.h"
#include "common/node_ref_util.h"
#include "common/node_ref_format.h"
#include "translation.h"

#include <atlbase.h>
#include <wtl/atlapp.h>
#include <atlstr.h>
#include <atlwin.h>

class LimitsDialog : protected ATL::CDialogImpl<LimitsDialog> {
 public:
  explicit LimitsDialog(TaskManager& task_manager, const NodeRef& node);

  bool Execute(DialogService& dialog_service);
  
 protected:
  friend class ATL::CDialogImpl<LimitsDialog>;

  enum { IDD = IDD_LIMITS };

  BEGIN_MSG_MAP(LimitsDialog)
    MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
    COMMAND_ID_HANDLER(IDOK, OnOK)
    COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
  END_MSG_MAP()

  LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
  LRESULT OnOK(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
  LRESULT OnCancel(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

 private:
  const NodeRef node_;
  TaskManager& task_manager_;
};

LimitsDialog::LimitsDialog(TaskManager& task_manager, const NodeRef& node)
    : task_manager_{task_manager},
      node_{std::move(node)} {
}

bool LimitsDialog::Execute(DialogService& dialog_service) {
  return DoModal(static_cast<DialogServiceViews&>(dialog_service).GetParentView()) == IDOK;
}

LRESULT LimitsDialog::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/,
                                   LPARAM /*lParam*/, BOOL& /*bHandled*/) {
  CenterWindow(GetParent());

  SetDlgItemText(IDC_DESC, base::SysNativeMBToWide(node_.display_name().text()).c_str());

  auto lolo = node_[id::AnalogItemType_LimitLoLo].value();
  auto hihi = node_[id::AnalogItemType_LimitHiHi].value();
  auto lo = node_[id::AnalogItemType_LimitLo].value();
  auto hi = node_[id::AnalogItemType_LimitHi].value();

  SetDlgItemText(IDC_LIMIT_LOLO, FormatValue(node_, lolo, {}, 0).c_str());
  SetDlgItemText(IDC_LIMIT_HIHI, FormatValue(node_, hihi, {}, 0).c_str());

  SetDlgItemText(IDC_LIMIT_LO, FormatValue(node_, lo, {}, 0).c_str());
  SetDlgItemText(IDC_LIMIT_HI, FormatValue(node_, hi, {}, 0).c_str());

  return TRUE;
}

LRESULT LimitsDialog::OnOK(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
  ATL::CString str;
  GetDlgItemText(IDC_LIMIT_LO, str);
  auto limit_lo = str.IsEmpty() ? scada::Variant() : ParseWithDefault(str.GetString(), 0.0);
  GetDlgItemText(IDC_LIMIT_HI, str);
  auto limit_hi = str.IsEmpty() ? scada::Variant() : ParseWithDefault(str.GetString(), 0.0);
  GetDlgItemText(IDC_LIMIT_LOLO, str);
  auto limit_lolo = str.IsEmpty() ? scada::Variant() : ParseWithDefault(str.GetString(), 0.0);
  GetDlgItemText(IDC_LIMIT_HIHI, str);
  auto limit_hihi = str.IsEmpty() ? scada::Variant() : ParseWithDefault(str.GetString(), 0.0);

  scada::NodeProperties properties;
  /*properties.emplace_back(id::AnalogItemType_LimitLo, limit_lo);
  properties.emplace_back(id::AnalogItemType_LimitHi, limit_hi);
  properties.emplace_back(id::AnalogItemType_LimitLoLo, limit_lolo);
  properties.emplace_back(id::AnalogItemType_LimitHiHi, limit_hihi);*/

  scada::Variant limit_hi2{limit_lo};

  task_manager_.PostUpdateTask(node_.id(), {}, std::move(properties));

  EndDialog(IDOK);
  return 0;
}

LRESULT LimitsDialog::OnCancel(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
  EndDialog(IDCANCEL);
  return 0;
}

void ShowLimitsDialog(DialogService& dialog_service, const NodeRef& node, TaskManager& task_manager) {
  LimitsDialog(task_manager, node).Execute(dialog_service);
}