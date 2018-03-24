#include "commands/time_range_dialog.h"

#include "base/format_time.h"
#include "common_resources.h"
#include "services/profile.h"

#include <algorithm>

using std::max;
using std::min;

#include <atlbase.h>

#include <atlapp.h>
#include <atlctrls.h>
#include <atlframe.h>

namespace {

SYSTEMTIME ToSystemTime(base::Time time) {
  SYSTEMTIME st = {};
  if (!time.is_null()) {
    FILETIME ft = time.ToFileTime();
    FILETIME lft;
    FileTimeToLocalFileTime(&ft, &lft);
    FileTimeToSystemTime(&lft, &st);
  }
  return st;
}

}  // namespace

class TimeRangeDialog : public ATL::CDialogImpl<TimeRangeDialog>,
                        public WTL::CDialogResize<TimeRangeDialog> {
 public:
  explicit TimeRangeDialog(Profile& profile);

  TimeRange time_range;
  bool time_required = false;

 protected:
  friend class ATL::CDialogImpl<TimeRangeDialog>;

  BEGIN_MSG_MAP(TimeRangeDialog)
  MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
  COMMAND_ID_HANDLER(IDOK, OnOK)
  COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
  COMMAND_HANDLER(IDC_TIME_CHECK, BN_CLICKED, OnTimeCheckClicked)
  CHAIN_MSG_MAP(CDialogResize<TimeRangeDialog>)
  END_MSG_MAP()

  BEGIN_DLGRESIZE_MAP(TimeRangeDialog)
  DLGRESIZE_CONTROL(IDC_START_DATE, DLSZ_SIZE_X | DLSZ_SIZE_Y)
  DLGRESIZE_CONTROL(IDC_START_TIME_CHECK, DLSZ_MOVE_X)
  DLGRESIZE_CONTROL(IDC_START_TIME, DLSZ_MOVE_X)
  DLGRESIZE_CONTROL(IDC_END_TIME_CHECK, DLSZ_MOVE_X)
  DLGRESIZE_CONTROL(IDC_END_TIME, DLSZ_MOVE_X)
  DLGRESIZE_CONTROL(IDC_TIME_CHECK, DLSZ_MOVE_X)
  DLGRESIZE_CONTROL(IDOK, DLSZ_MOVE_X | DLSZ_MOVE_Y)
  DLGRESIZE_CONTROL(IDCANCEL, DLSZ_MOVE_X | DLSZ_MOVE_Y)
  END_DLGRESIZE_MAP()

  LRESULT OnInitDialog(UINT /*uMsg*/,
                       WPARAM /*wParam*/,
                       LPARAM /*lParam*/,
                       BOOL& /*bHandled*/);
  LRESULT OnOK(WORD /*wNotifyCode*/,
               WORD /*wID*/,
               HWND /*hWndCtl*/,
               BOOL& /*bHandled*/);
  LRESULT OnCancel(WORD /*wNotifyCode*/,
                   WORD /*wID*/,
                   HWND /*hWndCtl*/,
                   BOOL& /*bHandled*/);
  LRESULT OnTimeCheckClicked(WORD /*wNotifyCode*/,
                             WORD /*wID*/,
                             HWND /*hWndCtl*/,
                             BOOL& /*bHandled*/);

  static const unsigned IDD = IDD_TIME_RANGE;

 private:
  struct Group {
    WTL::CDateTimePickerCtrl time_edit;
  };

  // Setup controls from data members.
  void SetTimeBound(Group& group, base::Time time);
  // Read control data to data member values.
  base::Time GetTimeBound(const Group& group,
                          const SYSTEMTIME& date,
                          bool dates) const;

  void UpdateTimeEditEnabled();

  Profile& profile_;

  Group start_group_;
  Group end_group_;
  WTL::CMonthCalendarCtrl calendar_;
  WTL::CButton time_checkbox_;
};

TimeRangeDialog::TimeRangeDialog(Profile& profile) : profile_{profile} {}

LRESULT TimeRangeDialog::OnInitDialog(UINT /*uMsg*/,
                                      WPARAM /*wParam*/,
                                      LPARAM /*lParam*/,
                                      BOOL& bHandled) {
  DlgResize_Init();

  {
    auto& settings = profile_.time_range_dialog;
    if (settings.width && settings.height)
      SetWindowPos(nullptr, 0, 0, settings.width, settings.height,
                   SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
    CenterWindow(GetParent());
  }

  calendar_ = GetDlgItem(IDC_START_DATE);
  calendar_.SetMaxSelCount(std::numeric_limits<int>::max());
  time_checkbox_ = GetDlgItem(IDC_TIME_CHECK);
  start_group_.time_edit = GetDlgItem(IDC_START_TIME);
  end_group_.time_edit = GetDlgItem(IDC_END_TIME);

  auto [start, end] = GetTimeRangeBounds(time_range);
  SYSTEMTIME times[2] = {ToSystemTime(start), ToSystemTime(end)};
  calendar_.SetSelRange(times);

  time_checkbox_.SetCheck(time_required || !time_range.dates);
  if (time_required)
    time_checkbox_.EnableWindow(FALSE);
  UpdateTimeEditEnabled();

  SetTimeBound(start_group_, time_range.start);
  SetTimeBound(end_group_, time_range.end);

  bHandled = FALSE;
  return 1;
}

LRESULT TimeRangeDialog::OnOK(WORD /*wNotifyCode*/,
                              WORD /*wID*/,
                              HWND /*hWndCtl*/,
                              BOOL& /*bHandled*/) {
  time_range.command_id = ID_TIME_RANGE_CUSTOM;

  SYSTEMTIME times[2] = {};
  calendar_.GetSelRange(times);

  time_range.dates = !time_required && time_checkbox_.GetCheck() != BST_CHECKED;
  time_range.start = GetTimeBound(start_group_, times[0], time_range.dates);
  time_range.end = GetTimeBound(end_group_, times[1], time_range.dates);

  {
    auto& settings = profile_.time_range_dialog;
    RECT rect = {};
    GetWindowRect(&rect);
    settings.width = rect.right - rect.left;
    settings.height = rect.bottom - rect.top;
  }

  EndDialog(IDOK);
  return 0;
}

LRESULT TimeRangeDialog::OnCancel(WORD /*wNotifyCode*/,
                                  WORD /*wID*/,
                                  HWND /*hWndCtl*/,
                                  BOOL& /*bHandled*/) {
  EndDialog(IDCANCEL);
  return 0;
}

LRESULT TimeRangeDialog::OnTimeCheckClicked(WORD /*wNotifyCode*/,
                                            WORD wID,
                                            HWND /*hWndCtl*/,
                                            BOOL& /*bHandled*/) {
  UpdateTimeEditEnabled();
  return 0;
}

void TimeRangeDialog::SetTimeBound(Group& group, base::Time time) {
  SYSTEMTIME st = ToSystemTime(time);
  group.time_edit.SetSystemTime(GDT_VALID, &st);
}

base::Time TimeRangeDialog::GetTimeBound(const Group& group,
                                         const SYSTEMTIME& date,
                                         bool dates) const {
  SYSTEMTIME st = date;

  SYSTEMTIME time_part = {};
  if (!dates)
    group.time_edit.GetSystemTime(&time_part);
  st.wHour = time_part.wHour;
  st.wMinute = time_part.wMinute;
  st.wSecond = time_part.wSecond;
  st.wMilliseconds = time_part.wMilliseconds;

  FILETIME ft, lft;
  SystemTimeToFileTime(&st, &lft);
  LocalFileTimeToFileTime(&lft, &ft);
  return base::Time::FromFileTime(ft);
}

void TimeRangeDialog::UpdateTimeEditEnabled() {
  BOOL enable = time_checkbox_.GetCheck() == BST_CHECKED;
  start_group_.time_edit.EnableWindow(enable);
  end_group_.time_edit.EnableWindow(enable);
}

bool ShowTimeRangeDialog(Profile& profile,
                         TimeRange& range,
                         bool time_required) {
  TimeRangeDialog dialog{profile};
  dialog.time_range = range;
  dialog.time_required = time_required;
  if (dialog.DoModal() != IDOK)
    return false;

  range = dialog.time_range;
  return true;
}
