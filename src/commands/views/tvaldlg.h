#pragma once

#include "common_resources.h"

#include <atlbase.h>
#include <wtl/atlapp.h>
#include <atlwin.h>
#include <wtl/atlctrls.h>

class TimeValDlg : public CDialogImpl<TimeValDlg>
{
public:
	enum { IDD = IDD_TIMEVAL };

	WTL::CDateTimePickerCtrl	wnd_date;
	WTL::CDateTimePickerCtrl	wnd_time;
	base::Time start_time;

	BEGIN_MSG_MAP(TimeValDlg)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		COMMAND_ID_HANDLER(IDOK, OnCloseCmd)
		COMMAND_ID_HANDLER(IDCANCEL, OnCloseCmd)
	END_MSG_MAP()

// Handler prototypes (uncomment arguments if needed):
//	LRESULT MessageHandler(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
//	LRESULT CommandHandler(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
//	LRESULT NotifyHandler(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/)

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	{
		CenterWindow(GetParent());

		wnd_date = GetDlgItem(IDC_DATE);
		wnd_time = GetDlgItem(IDC_TIME);

    SYSTEMTIME st = {0};
    FILETIME lft;
    FILETIME ft = start_time.ToFileTime();
    ::FileTimeToLocalFileTime(&ft, &lft);
    ::FileTimeToSystemTime(&lft, &st);

		wnd_date.SetSystemTime(GDT_VALID, &st);
		wnd_time.SetSystemTime(GDT_VALID, &st);

		return 0;
	}

	LRESULT OnCloseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		if (wID == IDOK) {
			SYSTEMTIME date, time;
			wnd_date.GetSystemTime(&date);
			wnd_time.GetSystemTime(&time);

      SYSTEMTIME st = date;
			st.wHour = time.wHour;
			st.wMinute = time.wMinute;
			st.wSecond = time.wSecond;
			st.wMilliseconds = time.wMilliseconds;

      FILETIME ft, lft;
      ::SystemTimeToFileTime(&st, &lft);
      ::LocalFileTimeToFileTime(&lft, &ft);
      start_time = base::Time::FromFileTime(ft);
		}

		EndDialog(wID);
		return 0;
	}
};
