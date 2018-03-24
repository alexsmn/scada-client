#pragma once

#include "common_resources.h"
#include "base/color.h"

#include <algorithm>

using std::min;
using std::max;

#include <atlbase.h>
#include <atlapp.h>
#include <atlframe.h>
#include <atlctrls.h>

class GraphSetupDialog : public ATL::CDialogImpl<GraphSetupDialog>,
	                       protected WTL::COwnerDraw<GraphSetupDialog>
{
public:
	enum { IDD = IDD_GRAPH_SETUP };

	WTL::CComboBox	wnd_color;
	SkColor color;
	int			line_weight_;

 protected:
  friend class WTL::COwnerDraw<GraphSetupDialog>;

	void DrawItem(LPDRAWITEMSTRUCT /*lpDrawItemStruct*/);

	BEGIN_MSG_MAP(GraphSetupDialog)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		COMMAND_ID_HANDLER(IDOK, OnCloseCmd)
		COMMAND_ID_HANDLER(IDCANCEL, OnCloseCmd)
		CHAIN_MSG_MAP(WTL::COwnerDraw<GraphSetupDialog>)
	END_MSG_MAP()

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnCloseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
};
