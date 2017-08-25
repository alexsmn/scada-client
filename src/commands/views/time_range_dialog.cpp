#include "client/commands/time_range_dialog.h"

#include <algorithm>

using std::min;
using std::max;

#include <atlbase.h>
#include <wtl/atlapp.h>
#include <wtl/atlctrls.h>

#include "client/common_resources.h"
#include "client/dialog_service.h"

class TimeRangeDialog : public ATL::CDialogImpl<TimeRangeDialog> {
 public:
  TimeRangeDialog();

  TimeRange GetTimeRange() const;

  void SetTimeRange(const TimeRange& range);

protected:
  friend class ATL::CDialogImpl<TimeRangeDialog>;

  BEGIN_MSG_MAP(TimeRangeDialog)
    MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
    COMMAND_ID_HANDLER(IDOK, OnOK)
    COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
    COMMAND_HANDLER(IDC_START_TIME_CHECK, BN_CLICKED, OnTimeCheckClicked)
    COMMAND_HANDLER(IDC_END_TIME_CHECK, BN_CLICKED, OnTimeCheckClicked)
  END_MSG_MAP()

  LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
  LRESULT OnOK(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
  LRESULT OnCancel(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
  LRESULT OnTimeCheckClicked(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

  static unsigned IDD;

private:
  struct Group {
    base::Time time;
    bool date_only;
    WTL::CMonthCalendarCtrl date_calendar;
    WTL::CButton time_checkbox;
    WTL::CDateTimePickerCtrl time_edit;
  };

  // Setup controls from data members.
  void LoadGroup(Group& group);
  // Read control data to data member values.
  void SaveGroup(Group& group);

  void UpdateTimeEditEnabled(Group& group);

  Group	start_group_;
  Group	end_group_;
};

unsigned TimeRangeDialog::IDD = IDD_TIME_RANGE;

TimeRangeDialog::TimeRangeDialog()
{
}

TimeRange TimeRangeDialog::GetTimeRange() const
{
	TimeRange range;
	range.start.time		= start_group_.time;
	range.start.date_only	= start_group_.date_only;
	range.end.time			= end_group_.time;
	range.end.date_only		= end_group_.date_only;
	return range;
}

void TimeRangeDialog::SetTimeRange(const TimeRange& range)
{
	start_group_.time		= range.start.time;
	start_group_.date_only	= range.start.date_only;
	end_group_.time			= range.end.time;
	end_group_.date_only	= range.end.date_only;
}

LRESULT TimeRangeDialog::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
{
	CenterWindow(GetParent());

	start_group_.date_calendar	= GetDlgItem(IDC_START_DATE);
	start_group_.time_checkbox	= GetDlgItem(IDC_START_TIME_CHECK);
	start_group_.time_edit		= GetDlgItem(IDC_START_TIME);
	end_group_.date_calendar	= GetDlgItem(IDC_END_DATE);
	end_group_.time_checkbox	= GetDlgItem(IDC_END_TIME_CHECK);
	end_group_.time_edit		= GetDlgItem(IDC_END_TIME);

	if (end_group_.time.is_null())
		end_group_.time = base::Time::Now();
	if (start_group_.time.is_null())
		start_group_.time = end_group_.time - base::TimeDelta::FromDays(1);

	LoadGroup(start_group_);
	LoadGroup(end_group_);

	bHandled = FALSE;
	return 1;
}

LRESULT TimeRangeDialog::OnOK(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	SaveGroup(start_group_);
	SaveGroup(end_group_);

	EndDialog(IDOK);
	return 0;
}

LRESULT TimeRangeDialog::OnCancel(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	EndDialog(IDCANCEL);
	return 0;
}

LRESULT TimeRangeDialog::OnTimeCheckClicked(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	if (wID == IDC_START_TIME_CHECK)
		UpdateTimeEditEnabled(start_group_);
	else if (wID == IDC_END_TIME_CHECK)
		UpdateTimeEditEnabled(end_group_);
	return 0;
}

void TimeRangeDialog::LoadGroup(Group& group) {
  SYSTEMTIME st = {0};
	FILETIME ft = group.time.ToFileTime();
  FILETIME lft;
  FileTimeToLocalFileTime(&ft, &lft);
  FileTimeToSystemTime(&lft, &st);

	group.date_calendar.SetCurSel(&st);
	group.time_checkbox.SetCheck(!group.date_only ? BST_CHECKED : BST_UNCHECKED);
	group.time_edit.SetSystemTime(0, &st);

	UpdateTimeEditEnabled(group);
}

void TimeRangeDialog::SaveGroup(Group& group) {
	SYSTEMTIME st;

	group.date_calendar.GetCurSel(&st);
	group.date_only = group.time_checkbox.GetCheck() != BST_CHECKED;

	if (!group.date_only) {
		SYSTEMTIME time_part;
		group.time_edit.GetSystemTime(&time_part);
		st.wHour			= time_part.wHour;
		st.wMinute		= time_part.wMinute;
		st.wSecond		= time_part.wSecond;
		st.wMilliseconds	= time_part.wMilliseconds;
	}

  FILETIME ft, lft;
  SystemTimeToFileTime(&st, &lft);
  LocalFileTimeToFileTime(&lft, &ft);

	group.time = base::Time::FromFileTime(ft);
}

void TimeRangeDialog::UpdateTimeEditEnabled(Group& group)
{
	group.time_edit.EnableWindow(group.time_checkbox.GetCheck() == BST_CHECKED);
}

bool ShowTimeRangeDialog(DialogService& dialog_service, TimeRange& range) {
  TimeRangeDialog dialog;
  dialog.SetTimeRange(range);
  if (dialog.DoModal(static_cast<DialogServiceViews&>(dialog_service).GetParentView()) != IDOK)
    return false;

  range = dialog.GetTimeRange();
  return true;
}
